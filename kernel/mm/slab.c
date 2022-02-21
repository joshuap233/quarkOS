//
// Created by pjs on 2021/5/2.
//
#include <mm/page_alloc.h>
#include <lib/qlib.h>
#include <mm/slab.h>
#include <mm/vm.h>


#define SLAB_INDEX(size) (log2(size)-2)
#define SLAB_MAGIC  0xac616749
#define SLAB_SIZE(idx)   (SLAB_MIN<<(idx))

#define chunk_foreach(chunk) for ((chunk) = info->chunk; chunk; (chunk) = (chunk)->next)

static void *__slab_alloc(list_head_t *head, list_head_t *full, uint16_t size);

static void add_slab(list_head_t *slab, uint16_t chunkSize);

static void __slab_free(void *addr, list_head_t *head);

static void recycle(list_head_t *head);

static struct allocator {
    list_head_t slab[LIST_SIZE]; // 部分分配的 slab
    list_head_t full_slab;       // 已经完全分配的 slab 列表
    list_head_t cache;
    struct spinlock lock;
} allocator;


void slab_init() {
    for (uint32_t i = 0; i < LIST_SIZE; ++i) {
        list_header_init(&allocator.slab[i]);
    }
    list_header_init(&allocator.cache);
    list_header_init(&allocator.full_slab);
    spinlock_init(&allocator.lock);

#ifdef TEST
    test_slab_alloc();
#endif //TEST
}


// 可以指定固定大小的 slab 分配器
void cache_alloc_create(struct slabCache *cache, size_t size) {
    list_header_init(&cache->head);
    list_header_init(&cache->cache_list);
    list_header_init(&cache->full_list);
    spinlock_init(&cache->lock);

    cache->size = size;
    list_add_next(&cache->head, &allocator.cache);
}

void *cache_alloc(struct slabCache *cache) {
    spinlock_lock(&cache->lock);
    void *ret= __slab_alloc(&cache->cache_list, &cache->full_list, cache->size);
    spinlock_unlock(&cache->lock);
    return ret;
}


void cache_free(struct slabCache *cache, void *addr) {
    spinlock_lock(&cache->lock);
    __slab_free(addr, &cache->cache_list);
    spinlock_unlock(&cache->lock);
}

// 内置的 slab 分配器(kmalloc)
void *slab_alloc(u16_t size) {
    assertk(size != 0 && size <= SLAB_MAX);

    if (size < SLAB_MIN)
        size = SLAB_MIN;

    size = fixSize(size);
    u16_t idx = SLAB_INDEX(size);
    spinlock_lock(&allocator.lock);
    void *ret= __slab_alloc(&allocator.slab[idx], &allocator.full_slab, size);
    spinlock_unlock(&allocator.lock);
    return ret;
}

void slab_free(void *addr) {
    struct page *page = va_get_page(PAGE_ADDR((ptr_t) addr));
    assertk(page && is_slab(page));

    slabInfo_t *info = &page->slab;
    uint16_t idx = SLAB_INDEX(info->size);
    spinlock_lock(&allocator.lock);
    __slab_free(addr, &allocator.slab[idx]);
    spinlock_unlock(&allocator.lock);
}

// slab 操作
static void __slab_free(void *addr, list_head_t *head) {
    ptr_t block = PAGE_ADDR((ptr_t) addr);
    struct page *page = va_get_page(block);

    assertk(page && is_slab(page));

    slabInfo_t *info = &page->slab;

#ifdef DEBUG
    assertk(info->magic == SLAB_MAGIC);
#endif // DEBUG

    assertk(info->n_allocated > 0);

    if (info->chunk == NULL) {
        list_del(&page->head);
        list_add_next(&page->head, head);
    }

    info->n_allocated--;
    chunkLink_t *ckHead = (chunkLink_t *) addr;
    ckHead->next = info->chunk;
    info->chunk = ckHead;
}


static void *__slab_alloc(list_head_t *head, list_head_t *full, uint16_t size) {
    if (list_empty(head)) {
        add_slab(head, size);
    }
    struct page *page = PAGE_ENTRY(head->next);

    wlock_lock(&page->rwlock);
    slabInfo_t *slabInfo = &page->slab;
    assertk(!list_empty(head) && slabInfo->chunk);

    slabInfo->n_allocated++;
    chunkLink_t *chunk = slabInfo->chunk;
    slabInfo->chunk = slabInfo->chunk->next;
    if (!slabInfo->chunk) {
        list_del(&page->head);
        list_add_next(&page->head, full);
    }
    wlock_unlock(&page->rwlock);

    return chunk;
}

static void add_slab(list_head_t *slab, uint16_t chunkSize) {
    struct page *page = __alloc_page(PAGE_SIZE);
    page->flag |= PG_SLAB;

    kvm_map(page, VM_PRES | VM_KRW);
    slabInfo_t *info = &page->slab;
    uint16_t cnt_unused = PAGE_SIZE / chunkSize;
    info->size = chunkSize;
    info->n_allocated = 0;
    info->chunk = page->data;
#ifdef DEBUG
    info->magic = SLAB_MAGIC;
#endif //DEBUG

    chunkLink_t *chunk = info->chunk;
    // 使用单向链表链接可用块
    for (int i = 0; i < cnt_unused - 1; ++i) {
        chunk->next = (chunkLink_t *) ((void *) chunk + chunkSize);
        chunk = chunk->next;
    }
    chunk->next = NULL;
    list_add_next(&page->head, slab);
}


u16_t slab_chunk_size(void *addr) {
    struct page *page = va_get_page(PAGE_ADDR((ptr_t) addr));
    assertk(page && is_slab(page));
    slabInfo_t *info = &page->slab;
    assertk(info->size <= SLAB_MAX);
    return info->size;
}


static void recycle(list_head_t *head) {
    list_head_t *hdr;
    struct page *page;

    list_for_each(hdr, head) {
        page = PAGE_ENTRY(hdr);
        wlock_lock(&page->rwlock);
        slabInfo_t *info = &page->slab;
        if (info->n_allocated == 0) {
            list_del(&page->head);
            __free_page(page);
            kvm_unmap(page);
        }
        wlock_unlock(&page->rwlock);
    }
}

// kvm_unmapPage 开启分页后才能使用,
// 因此 slab_recycle 需要在分页开启后测试
void slab_recycle() {
    spinlock_lock(&allocator.lock);
    for (int i = 0; i < LIST_SIZE; ++i) {
        recycle(&allocator.slab[i]);
    }
    spinlock_unlock(&allocator.lock);

    list_head_t *hdr;
    list_for_each(hdr, &allocator.cache) {
        struct slabCache *cache = container_of(hdr,struct slabCache,head);
        spinlock_lock(&cache->lock);
        recycle(hdr);
        spinlock_unlock(&cache->lock);
    }
}

#ifdef TEST

#define CHUNK_CNT(idx) (PAGE_SIZE/SLAB_SIZE(idx))

void *slabAddr[CHUNK_CNT(0)][2];

void except_chunk_size(u32_t idx, u32_t except) {
    size_t size = 0;
    list_head_t *hdr = &allocator.slab[idx];
    if (list_empty(hdr)) {
        assertk(0 == except);
        return;
    }

    slabInfo_t *info = SLAB_ENTRY(hdr->next);
    assertk(info->chunk);
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
    assertk(allocator.slab->next != PAGE_ENTRY(allocator.slab->next)->head.next);
    except_chunk_size(0, CHUNK_CNT(0));

    slab_free(slabAddr[0][1]);
    size_t size = 0;
    slabInfo_t *info = SLAB_ENTRY(PAGE_ENTRY(&allocator.slab[0])->head.next);
    assertk(info->chunk);

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
