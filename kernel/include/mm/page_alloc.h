//
// Created by pjs on 2021/2/1.
//

#ifndef QUARKOS_MM_PAGE_ALLOC_H
#define QUARKOS_MM_PAGE_ALLOC_H

#include <types.h>
#include <lib/list.h>
#include <mm/page.h>

#define MAX_ORDER 10

struct buddyAllocator{
    list_head_t root[MAX_ORDER + 1];
    ptr_t addr;
    ptr_t size;
    ptr_t freeSize;
    struct page *pages;
    u32_t pageCnt;        // 管理器管理的页面数量
};

struct page *__alloc_page(u32_t size);

ptr_t alloc_one_page();

ptr_t alloc_page(u32_t size);

void __free_page(struct page *page);

int32_t free_page(ptr_t addr);

ptr_t page_addr(struct page *page);

struct page *get_page(ptr_t addr);

void pmm_init_mm();


#ifdef TEST

void test_alloc();

#endif //TEST

#endif //QUARKOS_MM_PAGE_ALLOC_H
