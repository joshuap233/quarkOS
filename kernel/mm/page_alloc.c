//
// Created by pjs on 2021/2/1.
//

#include <types.h>
#include <lib/qlib.h>
#include <mm/mm.h>
#include <mm/page_alloc.h>
#include <mm/block_alloc.h>
#include <mm/page.h>

/*
 * 物理内存分配器
 * root[0] - root[10] 为 0 - 4M 内存块
 */
#define ORDER_SIZE(order) ((ptr_t)1<<(order)<<12)
#define ORDER_CNT(order)  (1<<(order))
static struct buddyAllocator allocator;


struct page *page_setup(struct page *pages, ptr_t addr, ptr_t size) {
    u32_t reSize = allocator.pageCnt - (pages - allocator.pages);
    assertk(reSize >= (size >> 12));

    size = PAGE_ADDR(size - (PAGE_ALIGN(addr) - addr));
    addr = PAGE_ALIGN(addr);

    struct page *page = pages;
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
            list_add_prev(&page->head, &allocator.root[i]);
            for (int k = 0; k < ORDER_CNT(i); ++k) {
                page->flag |= PG_Page;
                page++;
            }
        }
    }
    allocator.size += (page - pages) << 12;
    return page;
}

// 在内核分页开启前预留内存
void pmm_init_mm() {
    u32_t pageCnt = (block_end() - block_start()) >> 12;
    allocator.pageCnt = pageCnt;
    allocator.pages = (void *) block_alloc_align(
            pageCnt * sizeof(struct page),
                    sizeof(struct page)) + KERNEL_START;
}

void pmm_init() {
    list_head_t *hdr;
    blockInfo_t *info;
    struct page *page;

    allocator.size = 0;
    allocator.addr = PAGE_ALIGN(block_start());

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

    for (int j = 0; j <= MAX_ORDER; ++j) {
        list_header_init(&allocator.root[j]);
    }

    u32_t addr, size;
    page = allocator.pages;

    // 将物理页放入管理器
    list_for_each(hdr, &blockAllocator.head) {
        info = block_mem_entry(hdr);
        addr = info->addr;
        size = info->size;
        page = page_setup(page, addr, size);
    }
    allocator.pageCnt = page - allocator.pages;
    allocator.freeSize = allocator.size;

#ifdef TEST
    test_alloc();
#endif //TEST
}


// 获取页的物理地址
ptr_t page_addr(struct page *page) {
    ptr_t start = allocator.addr;
    return start + ((ptr_t) (page - allocator.pages) << 12);
}

// 使用物理地址找到页

struct page *get_page(ptr_t addr) {
    if ((addr & PAGE_MASK) != 0)
        return NULL;
    ptr_t start = allocator.addr;

    if (addr < start)
        return NULL;

    struct page *page = &allocator.pages[(addr - start) >> 12];

    assertk(page->magic == PAGE_MAGIC);
    return page;
}

struct page *get_buddy(struct page *page) {
    u32_t pgCnt = page->size >> 12;
    u32_t cnt = (page - allocator.pages) / pgCnt;
    struct page *buddy = (cnt & 1) ? page - pgCnt : page + pgCnt;
    if (allocator.pages + allocator.pageCnt <= buddy) {
        buddy = NULL;
        return buddy;
    }
    return buddy;
}

struct page *__alloc_page(u32_t size) {
    size = fixSize(size);
    assertk(size > 0 && size <= 4 * M && (size & PAGE_MASK) == 0);
    struct page *page = NULL, *new;
    list_head_t *root;
    u32_t logSize = log2(size >> 12);

    for (uint16_t i = logSize; i <= MAX_ORDER; ++i) {
        root = &allocator.root[i];
        if (!list_empty(root)) {
            page = PAGE_ENTRY(root->next);
            assertk(page_head(page));
            for (u64_t j = i - 1; j >= logSize && j <= MAX_ORDER; j--) {
                u32_t pageCnt = ORDER_CNT(j);
                new = page + pageCnt;
                assertk(!page_head(new));
                new->size = ORDER_SIZE(j);
                new->flag |= PG_Head;
                list_add_prev(&new->head, &allocator.root[j]);
            }
            break;
        }
    }
    assertk(page != NULL);
    allocator.size -= size;
    list_del(&page->head);
    page->size = size;
    page->ref_cnt = 1;
    return page;
}

ptr_t alloc_page(u32_t size) {
    struct page *page = __alloc_page(size);
    return page_addr(page);
}

ptr_t alloc_one_page() {
    return alloc_page(PAGE_SIZE);
}

void __free_page(struct page *page) {
    assertk(page->ref_cnt > 0);
    assertk(page->flag & PG_Head);

    page->ref_cnt--;
    if (page->ref_cnt > 0) return;

    u32_t order = log2(page->size >> 12);
    page->data = NULL;
    page->flag |= PG_Page | PG_Head;    // 清除其他 flag
    for (; page->ref_cnt == 0 && order <= MAX_ORDER; ++order) {
        struct page *buddy = get_buddy(page);
        if (!buddy ||                   // 部分页面没有buddy
            buddy->ref_cnt != 0 ||      // 被引用的页面
            buddy->size != page->size ||// buddy 的部分页面没有被释放
            order == MAX_ORDER) {       // 页面大小为 4M 不需要继续合并
            list_add_prev(&page->head, &allocator.root[order]);
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

// addr 为物理地址
int32_t free_page(ptr_t addr) {
    assertk((addr & PAGE_MASK) == 0);
    struct page *page = get_page(addr);
    if (!page) return -1;

    assertk(page->flag & PG_Head);

    __free_page(page);
    return 0;
}


#ifdef TEST


static u32_t page_cnt() {
    u32_t pageCnt = 0;
    for (int j = 0; j <= MAX_ORDER; ++j) {
        pageCnt += list_cnt(&allocator.root[j]) * ORDER_CNT(j);
    }
    return pageCnt;
}

static u32_t allocator_size() {
    u32_t size = 0;
    list_head_t *hdr;
    for (int j = 0; j <= MAX_ORDER; ++j) {
        u32_t orderSize = ORDER_SIZE(j);
        list_for_each(hdr, &allocator.root[j]) {
            struct page *page = PAGE_ENTRY(hdr);
            size += orderSize;
            assertk(page->size == orderSize);
        }
    }
    return size;
}

static ptr_t pages[MAX_ORDER];
static ptr_t pages2[20];

void test_alloc() {
    test_start
    u32_t pageCnt = page_cnt();
    u32_t size = allocator_size();
    ptr_t start = allocator.addr;
    struct page *page;
    for (int i = 0; i < 10; ++i) {
        page = get_page(start + (i << 12));
        assertk(page);
        assertk((start + (i << 12)) == page_addr(page));
    }

    assertk(size == (allocator.pageCnt << 12));
    for (int i = 0; i < MAX_ORDER; ++i) {
        pages[i] = alloc_page(ORDER_SIZE(i));
    }

    for (int i = 0; i < 20; ++i) {
        pages2[i] = alloc_page(PAGE_SIZE);
    }

    for (int i = 0; i < MAX_ORDER; ++i) {
        free_page(pages[i]);
    }

    for (int i = 0; i < 20; ++i) {
        free_page(pages2[i]);
    }

    assertk(allocator_size() == size);
    assertk(page_cnt() == pageCnt);
    test_pass
}


#endif //TEST