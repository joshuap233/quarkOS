//
// Created by pjs on 2021/5/2.
//
#include "mm/page_alloc.h"
#include "lib/qlib.h"
#include "mm/slab.h"
#include "mm/kvm.h"

#define SLAB_INDEX(size) (log2(size)-2)
#define SLAB_INFO_MAGIC 0xac616749
#define entry(ptr) list_entry(ptr,slabInfo_t,head)
#define SLAB_SIZE(idx)   (SLAB_MIN<<(idx))

#define chunk_foreach(chunk) for ((chunk) = info->chunk; chunk; (chunk) = (chunk)->next)

static void *_slab_alloc(list_head_t *head, list_head_t *full, uint16_t size);

static void add_slab(list_head_t *slab, uint16_t chunkSize);

static void _slab_free(void *addr, list_head_t *head);

static void slab_cache_init(struct slabCache *cache);

static void recycle(list_head_t *head);

struct allocator {
    list_head_t slab[LIST_SIZE]; // 部分分配的 slab
    list_head_t full_slab;       // 已经完全分配的 slab 列表

    list_head_t cache;
} allocator;


void slab_init() {
    for (uint32_t i = 0; i < LIST_SIZE; ++i) {
        list_header_init(&allocator.slab[i]);
    }
    list_header_init(&allocator.cache);
    list_header_init(&allocator.full_slab);
#ifdef TEST
    test_slab_alloc();
#endif //TEST
}

static void slab_cache_init(struct slabCache *cache) {
    list_header_init(&cache->head);
    list_header_init(&cache->cache_list);
    list_header_init(&cache->full_list);
    cache->size = 0;
}

// 可以指定固定大小的 slab 分配器

void cache_alloc_create(struct slabCache *cache, size_t size) {
    slab_cache_init(cache);
    cache->size = size;
    list_add_next(&cache->head, &allocator.cache);
}

void *cache_alloc(struct slabCache *cache) {
    return _slab_alloc(&cache->cache_list, &cache->full_list, cache->size);
}


void cache_free(struct slabCache *cache, void *addr) {
    _slab_free(addr, &cache->cache_list);
}

// 内置的 slab 分配器(kmalloc)

void *slab_alloc(uint16_t size) {
    if (size < SLAB_MIN) size = SLAB_MIN;
    if (!IS_POWER_OF_2(size))
        size = fixSize(size);

    uint16_t idx = SLAB_INDEX(size);
    return _slab_alloc(&allocator.slab[idx], &allocator.full_slab, size);
}


void slab_free(void *addr) {
    slabInfo_t *info = (void *) PAGE_ADDR((ptr_t) addr);
    uint16_t idx = SLAB_INDEX(info->size);

    _slab_free(addr, &allocator.slab[idx]);
}

// slab 操作
static void _slab_free(void *addr, list_head_t *head) {
    slabInfo_t *info = (void *) PAGE_ADDR((ptr_t) addr);
    assertk(info->magic == SLAB_INFO_MAGIC && info->n_allocated > 0);

    if (info->chunk == NULL) {
        list_del(&info->head);
        list_add_next(&info->head, head);
    }

    info->n_allocated--;
    chunkLink_t *ckHead = (chunkLink_t *) addr;
    ckHead->next = info->chunk;
    info->chunk = ckHead;
}


static void *_slab_alloc(list_head_t *head, list_head_t *full, uint16_t size) {
    if (list_empty(head)) {
        add_slab(head, size);
    }

    slabInfo_t *slabInfo = entry(head->next);
    assertk(!list_empty(head) && slabInfo->chunk);

    slabInfo->n_allocated++;
    chunkLink_t *chunk = slabInfo->chunk;
    slabInfo->chunk = slabInfo->chunk->next;
    if (!slabInfo->chunk) {
        list_del(&slabInfo->head);
        list_add_next(&slabInfo->head, full);
    }
    return chunk;
}


static void add_slab(list_head_t *slab, uint16_t chunkSize) {
    ptr_t addr = pm_alloc_page();
    assertk((addr & PAGE_MASK) == 0);
    kvm_mapPage(addr, addr, VM_PRES | VM_KW);

    // slabInfo 结构位于当前 slab 页的头部
    uint16_t cnt_unused = (PAGE_SIZE - sizeof(slabInfo_t)) / chunkSize;
    slabInfo_t *info = (void *) addr;
    info->n_allocated = 0;

    info->chunk = (void *) addr + sizeof(slabInfo_t);
    info->magic = SLAB_INFO_MAGIC;
    info->size = chunkSize;

    chunkLink_t *chunk = info->chunk;
    // 使用单向链表链接可用块
    for (int i = 0; i < cnt_unused - 1; ++i) {
        chunk->next = (chunkLink_t *) ((void *) chunk + chunkSize);
        chunk = chunk->next;
    }
    chunk->next = NULL;
    list_add_next(&info->head, slab);
}


u16_t slab_chunk_size(void *addr) {
    slabInfo_t *info = (void *) PAGE_ADDR((ptr_t) addr);
    return info->size;
}

u32_t fixSize(u32_t size) {
    size |= size >> 1;
    size |= size >> 2;
    size |= size >> 4;
    size |= size >> 8;
    size |= size >> 16;
    return size + 1;
}


static void recycle(list_head_t *head) {
    slabInfo_t *info;
    list_head_t *hdr;
    list_for_each(hdr, head) {
        info = entry(hdr);
        if (info->n_allocated == 0) {
            list_del(&info->head);
            pm_free((ptr_t) info);
            kvm_unmapPage((ptr_t) info);
        }
    };
}

// kvm_unmapPage 开启分页后才能使用,
// 因此 slab_recycle 需要在分页开启后测试
void slab_recycle() {
    for (int i = 0; i < LIST_SIZE; ++i) {
        recycle(&allocator.slab[i]);
    }

    list_head_t *hdr;
    list_for_each(hdr, &allocator.cache) {
        recycle(hdr);
    }
}

#ifdef TEST

#define CHUNK_CNT(idx) ((PAGE_SIZE-sizeof(slabInfo_t))/SLAB_SIZE(idx))

void *slabAddr[CHUNK_CNT(0)][2];

void except_chunk_size(u32_t idx, u32_t except) {
    size_t size = 0;
    slabInfo_t *info = entry(allocator.slab[idx].next);
    assertk(info && info->chunk);
    chunkLink_t *chunk;

    chunk_foreach(chunk) {
        size++;
    }
    assertk(size == except);
}

void test_slab_alloc() {
    test_start

    for (u32_t i = 0; i < LIST_SIZE; ++i) {
        slabAddr[i][0] = slab_alloc(SLAB_MIN << i);
        except_chunk_size(i, CHUNK_CNT(i) - 1);
        slabAddr[i][1] = slab_alloc(SLAB_MIN << i);
        except_chunk_size(i, CHUNK_CNT(i) - 2);
    }

    for (u32_t i = 0; i < LIST_SIZE; ++i) {
        slab_free(slabAddr[i][0]);
        except_chunk_size(i, CHUNK_CNT(i) - 1);
        slab_free(slabAddr[i][1]);
        except_chunk_size(i, CHUNK_CNT(i));
    }

    // 测试使用完一整页后 slab 扩展
    for (u32_t i = 0; i < CHUNK_CNT(0); ++i) {
        slabAddr[i][0] = slab_alloc(SLAB_MIN);
    }

    assertk(!list_empty(&allocator.full_slab));
    assertk(list_empty(&allocator.slab[0]));

    slabAddr[0][1] = slab_alloc(SLAB_MIN);
    except_chunk_size(0, CHUNK_CNT(0) - 1);

    for (u32_t i = 0; i < CHUNK_CNT(0); ++i) {
        slab_free(slabAddr[i][0]);
    }
    assertk(!list_empty(&allocator.slab[0]));
    assertk(allocator.slab->next != entry(allocator.slab->next)->head.next)
    except_chunk_size(0, CHUNK_CNT(0));

    slab_free(slabAddr[0][1]);
    size_t size = 0;
    slabInfo_t *info = entry(entry(&allocator.slab[0])->head.next);
    assertk(info && info->chunk);

    chunkLink_t *chunk;
    chunk_foreach(chunk) {
        size++;
    }
    assertk(size == CHUNK_CNT(0));
    assertk(list_empty(&allocator.full_slab));

    test_pass
}

void test_slab_recycle() {
    test_start
    slab_recycle();
    for (int i = 0; i < LIST_SIZE; ++i) {
        assertk(list_empty(&allocator.slab[i]));
    }
    assertk(list_empty(&allocator.full_slab));
    test_pass
}

#endif //TEST
