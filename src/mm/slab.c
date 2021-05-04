//
// Created by pjs on 2021/5/2.
//
#include "mm/page_alloc.h"
#include "lib/qlib.h"
#include "mm/slab.h"
#include "mm/vmm.h"

#define SLAB_INDEX(size) (((size) >> 3)-1)
#define SLAB_INFO_MAGIC 0xac616749
#define entry(ptr) list_entry(ptr,slabInfo_t,head)


static void add_slab(uint16_t idx, uint16_t chunkSize);


struct allocator {
#define LIST_SIZE (SLAB_MAX/8)
    list_head_t *slab[LIST_SIZE];
    list_head_t full_slab;   // 已经完全分配的 slab 列表
} allocator;


void slab_init() {
    for (uint32_t i = 0; i < LIST_SIZE; ++i) {
        allocator.slab[i] = NULL;
    }
}


void *slab_alloc(uint16_t size) {
    size = MEM_ALIGN(size, 8);
    uint16_t idx = SLAB_INDEX(size);
    slabInfo_t *slabInfo = entry(allocator.slab[idx]);

    if (!slabInfo || !slabInfo->chunk)
        add_slab(idx, size);
    slabInfo->n_allocated++;
    chunkLink_t *chunk = slabInfo->chunk;
    slabInfo->chunk = slabInfo->chunk->next;
    if (!slabInfo->chunk) {
        list_add_next(&slabInfo->head, &allocator.full_slab);
        allocator.slab[idx] = slabInfo->head.next;
    }
    return chunk;
}

static void add_slab(uint16_t idx, uint16_t chunkSize) {
    ptr_t addr = pm_alloc_page();

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

    allocator.slab[idx] = &info->head;
    list_header_init(&info->head);
}

void slab_free(void *addr) {
    slabInfo_t *info = (void *) PAGE_ADDR((ptr_t) addr);
    assertk(info->magic == SLAB_INFO_MAGIC && info->n_allocated > 0);
    uint16_t idx = SLAB_INDEX(info->size);

    if (info->chunk == NULL) {
        list_del(&info->head);
        info->head.next = allocator.slab[idx];
        allocator.slab[idx] = &info->head;
    }

    info->n_allocated--;
    chunkLink_t *head = (chunkLink_t *) addr;
    head->next = info->chunk;
    info->chunk = head;
}

void slab_recycle() {
    // TODO:多余的页等待内存不足时,使用回收线程释放多余的 slab
}