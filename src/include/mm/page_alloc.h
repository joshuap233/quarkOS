//
// Created by pjs on 2021/2/1.
//

#ifndef QUARKOS_MM_PMM_H
#define QUARKOS_MM_PMM_H

#include "types.h"
#include "lib/list.h"

//typedef struct page {
//    struct list_head list;
//    struct address_space *mapping;
//    unsigned long index;
//    struct page *next_hash;
//    atomic_t count;
//    unsigned long flags; /* atomic flags, some possibly updated asynchronously */
//    struct list_head lru;
//    unsigned long age;
//    wait_queue_head_t wait;
//    struct page **pprev_hash;
//    struct buffer_head *buffers;
//    void *virtual; /* non-NULL if kmapped */
//    struct zone_struct *zone;
//} mem_map_t;

#define PMM_NULL 0xffffffff

ptr_t pm_alloc(u32_t size);

ptr_t pm_alloc_page();

u32_t pm_free(ptr_t addr);

u32_t pm_chunk_size(ptr_t addr);

#ifdef TEST

void test_alloc();

#endif //TEST

#endif //QUARKOS_MM_PMM_H
