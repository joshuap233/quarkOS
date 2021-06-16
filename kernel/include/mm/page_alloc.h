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
    struct mem_zone {
        list_head_t root[MAX_ORDER + 1];
        struct page *first;
        ptr_t addr;
        ptr_t size;
        ptr_t freeSize;
    } zone[2];
    struct page *pages;
    u32_t pageCnt;
};

struct page *__kalloc_page(u32_t size);

ptr_t kalloc_page(u32_t size);

ptr_t kalloc_one_page();

struct page *__alloc_page(u32_t size);

ptr_t alloc_one_page();

ptr_t alloc_page(u32_t size);

void __free_page(struct page *page);

void free_page(ptr_t addr);

ptr_t page_addr(struct page *page);

struct page *get_page(ptr_t addr);
struct page *get_page2(ptr_t addr);

void pmm_init_mm();


#ifdef TEST

void test_alloc();

#endif //TEST

#endif //QUARKOS_MM_PAGE_ALLOC_H
