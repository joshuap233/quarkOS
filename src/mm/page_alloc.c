//
// Created by pjs on 2021/2/1.
//
// TODO: 分配需要分配 3K, 5K, 9K...这种内存依然会造成大量内存碎片
// 可以将多余的内存划分给 slab 分配器?

#include <types.h>
#include <lib/qlib.h>
#include <mm/mm.h>
#include <mm/page_alloc.h>
#include <mm/block_alloc.h>
#include <mm/page.h>

/*
 * 物理内存分配器
 * zone 0 为第 3G 内存, zone 1 为 高 1G 内存
 * 内存不足 4G 则没有 zone 1,内核使用 zone 0 区域
 * root[0] - root[10] 为 0 - 4M 内存块
 */
#define ORDER_SIZE(order) ((ptr_t)1<<(order)<<12)
#define ORDER_CNT(order)  (1<<(order))
static struct buddyAllocator allocator;


//返回已经使用的页面数量
struct page *page_setup(struct page *pages, ptr_t addr, ptr_t size) {
    u32_t reSize = allocator.pageCnt - (pages - allocator.pages);
    assertk(reSize >= (size >> 12));

    size = PAGE_ADDR(size - (PAGE_ALIGN(addr) - addr));
    addr = PAGE_ALIGN(addr);

    struct page *page = pages;
    struct mem_zone *zone = addr >= HIGH_MEM ? &allocator.zone[1] : &allocator.zone[0];
    u32_t cnt;
    u32_t orderSize;


    for (int i = MAX_ORDER; size > 0 && i >= 0; --i) {
        orderSize = ORDER_SIZE(i);
        if (size < orderSize) continue;

        cnt = size / orderSize;
        size %= orderSize;
        for (u32_t j = 0; j < cnt; ++j) {
            page->flag |= PG_Head;
            page->size = orderSize;
            list_add_prev(&page->head, &zone->root[i]);
            for (int k = 0; k < ORDER_CNT(i); ++k) {
                page->flag |= PG_Page;
                page++;
            }
        }
    }
    zone->size += (page - pages) << 12;
    return page;
}

// 在内核分页开启前预留内存
void pmm_init_mm() {
    u32_t pageCnt = (block_end() - block_start()) >> 12;
    allocator.pageCnt = pageCnt;
    allocator.pages = (void *) block_alloc_align(pageCnt * sizeof(struct page), sizeof(struct page)) + HIGH_MEM;
}

void pmm_init() {
    list_head_t *hdr;
    blockInfo_t *info;
    struct page *page;

    allocator.zone[0].size = 0;
    allocator.zone[1].size = 0;

    allocator.zone[0].addr = PAGE_ALIGN(block_start());
    allocator.zone[1].addr = HIGH_MEM;

    allocator.zone[0].first = allocator.pages;
    allocator.zone[1].first = NULL;

    // 初始化页面
    page = allocator.pages;
    for (u64_t i = 0; i < allocator.pageCnt; ++i) {
        page->flag = PG_Hole | PG_Tail;
        page->ref_cnt = 0;
        page->size = 0;

#ifdef DEBUG
        page->magic = PAGE_MAGIC;
#endif //DEBUG

        rwlock_init(&page->rwlock);
        list_header_init(&page->head);
        page++;
    }

    // 初始化 zone
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j <= MAX_ORDER; ++j) {
            list_header_init(&allocator.zone[i].root[j]);
        }
    }

    u32_t addr, size;
    page = allocator.pages;

    // 将 page 放入 zone
    list_for_each(hdr, &blockAllocator.head) {
        info = block_mem_entry(hdr);
        addr = info->addr;
        size = info->size;

        if (addr < HIGH_MEM && addr + size > HIGH_MEM) {
            page = page_setup(page, addr, HIGH_MEM - addr);
            addr = HIGH_MEM;
            size = addr + size - HIGH_MEM;
            allocator.zone[1].first = page;
        }
        page = page_setup(page, addr, size);
    }
    allocator.pageCnt = page - allocator.pages;
    allocator.zone[0].freeSize = allocator.zone[0].size;
    allocator.zone[1].freeSize = allocator.zone[1].size;

#ifdef TEST
    test_alloc();
#endif //TEST
}

INLINE struct mem_zone *get_zone(struct page *page) {
    struct mem_zone *zone = &allocator.zone[1];
    return (zone->first && zone->first < page)
           ? &allocator.zone[1]
           : &allocator.zone[0];
}

ptr_t page_addr(struct page *page) {
    ptr_t start = allocator.zone[0].addr;
    return start + ((ptr_t) (page - allocator.pages) << 12);
}

struct page *get_page(ptr_t addr) {
    assertk((addr & PAGE_MASK) == 0);
    ptr_t start = allocator.zone[0].addr;
    assertk(addr >= start);
    struct page *page = &allocator.pages[(addr - start) >> 12];
#ifdef DEBUG
    assertk(page->magic == PAGE_MAGIC);
#endif //DEBUG
    return page;
}


struct page *get_page2(ptr_t addr) {
    // 暂时懒得处理 get_page 的 assert,就分两个函数吧...
    if ((addr & PAGE_MASK) != 0)
        return NULL;
    ptr_t start = allocator.zone[0].addr;

    if (addr < start)
        return NULL;

    struct page *page = &allocator.pages[(addr - start) >> 12];

    if (page->magic != PAGE_MAGIC)
        return NULL;
    return page;
}

INLINE struct page *get_buddy(struct page *page) {
    struct mem_zone *zone = get_zone(page);
    u32_t pgCnt = page->size >> 12;
    u32_t cnt = (page - zone->first) / pgCnt;
    struct page *buddy = (cnt & 1) ? page - pgCnt : page + pgCnt;
    if (allocator.pages + allocator.pageCnt <= buddy) {
        buddy = NULL;
        return buddy;
    }
    return buddy;
}

static struct page *__alloc_pages(struct mem_zone *zone, u32_t size) {
    size = fixSize(size);
    assertk(size > 0 && size <= 4 * M);
    struct page *page = NULL, *new;
    list_head_t *root;
    u32_t logSize = log2(size >> 12);

    for (uint16_t i = logSize; i <= MAX_ORDER; ++i) {
        root = &zone->root[i];
        if (!list_empty(root)) {
            page = PAGE_ENTRY(root->next);
            assertk(page_head(page));
            for (u64_t j = i - 1; j >= logSize && j <= MAX_ORDER; j--) {
                u32_t pageCnt = ORDER_CNT(j);
                new = page + pageCnt;
                assertk(!page_head(new));
                new->size = ORDER_SIZE(j);
                new->flag |= PG_Head;
                list_add_prev(&new->head, &zone->root[j]);
            }
            break;
        }
    }
    assertk(page != NULL);
    zone->size -= size;
    list_del(&page->head);
    page->size = size;
    page->ref_cnt = 1;
    return page;
}

struct page *__alloc_page(u32_t size) {
    return __alloc_pages(&allocator.zone[0], size);
}

// 分配 0 -3G 内存
ptr_t alloc_page(u32_t size) {
    struct page *page = __alloc_page(size);
    return page_addr(page);
}

ptr_t alloc_one_page() {
    return alloc_page(PAGE_SIZE);
}

// 分配 3-4 G 内存
struct page *__kalloc_page(u32_t size) {
    struct mem_zone *zone =
            allocator.zone[0].freeSize > size ?
            &allocator.zone[0] : &allocator.zone[1];
    return __alloc_pages(zone, size);
}

ptr_t kalloc_page(u32_t size) {
    struct page *page = __kalloc_page(size);
    return page_addr(page);
}

ptr_t kalloc_one_page() {
    return kalloc_page(PAGE_SIZE);
}

void __free_page(struct page *page) {
    assertk(page->ref_cnt > 0);
    assertk(page->flag & PG_Head);

    page->ref_cnt--;
    if (page->ref_cnt > 0) return;

    struct mem_zone *zone = get_zone(page);
    u32_t order = log2(page->size >> 12);
    page->data = NULL;
    page->flag |= PG_Page | PG_Head; // 清除其他 flag
    for (; page->ref_cnt == 0 && order <= MAX_ORDER; ++order) {
        struct page *buddy = get_buddy(page);
        if (!buddy ||                   // 部分页面没有buddy
            buddy->ref_cnt != 0 ||      // 被引用的页面
            buddy->size != page->size ||// buddy 的部分页面没有被释放
            order == MAX_ORDER) {       // 页面大小为 4M 不需要继续合并
            list_add_prev(&page->head, &zone->root[order]);
            break;
        }
        list_del(&buddy->head);
        struct page *tmp = MAX(buddy, page);
        page = MIN(buddy, page);
        assertk(page + (page->size >> 12) == tmp);

        CLEAR_BIT(tmp->flag, PG_HEAD_BIT);
        list_header_init(&tmp->head);

        page->size <<= 1;
    }
}

void free_page(ptr_t addr) {
    assertk((addr & PAGE_MASK) == 0);
    struct page *page = get_page(addr);

    assertk(page->flag & PG_Head);

    __free_page(page);
}


#ifdef TEST


static u32_t page_cnt() {
    u32_t pageCnt = 0;
    for (int i = 0; i < 2; ++i) {
        if (allocator.zone[i].freeSize <= 0) continue;
        for (int j = 0; j <= MAX_ORDER; ++j) {
            pageCnt += list_cnt(&allocator.zone[i].root[j]);
        }
    }
    return pageCnt;
}

static u32_t allocator_size() {
    u32_t size = 0;
    list_head_t *hdr;
    for (int i = 0; i < 2; ++i) {
        if (allocator.zone[i].freeSize <= 0) continue;
        for (int j = 0; j <= MAX_ORDER; ++j) {
            u32_t orderSize = ORDER_SIZE(j);
            list_for_each(hdr, &allocator.zone[i].root[j]) {
                struct page *page = PAGE_ENTRY(hdr);
                size += orderSize;
                assertk(page->size == orderSize);
            }
        }
    }
    return size;
}

static ptr_t pages[MAX_ORDER];
static ptr_t pages2[20];

void test_alloc() {
    u32_t pageCnt = page_cnt();
    u32_t size = allocator_size();
    ptr_t start = allocator.zone[0].addr;
    struct page *page;
    for (int i = 0; i < 10; ++i) {
        page = get_page(start + (i << 12));
        assertk((start + (i << 12)) == page_addr(page));
    }

    assertk(size == (allocator.pageCnt << 12));
    for (int i = 0; i < MAX_ORDER; ++i) {
        pages[i] = kalloc_page(ORDER_SIZE(i));
    }

    for (int i = 0; i < 20; ++i) {
        pages2[i] = kalloc_page(PAGE_SIZE);
    }

    for (int i = 0; i < MAX_ORDER; ++i) {
        free_page(pages[i]);
    }

    for (int i = 0; i < 20; ++i) {
        free_page(pages2[i]);
    }

    assertk(allocator_size() == size);
    assertk(page_cnt() == pageCnt);
}


#endif //TEST