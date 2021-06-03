//
// Created by pjs on 2021/5/4.
//

#ifndef QUARKOS_SLAB_H
#define QUARKOS_SLAB_H

#include <types.h>
#include <mm/mm.h>
#include <lib/list.h>
#include <mm/page.h>

struct slabCache {
    list_head_t head;
    list_head_t cache_list;
    list_head_t full_list;
    size_t size;
};

void *slab_alloc(uint16_t size);

void slab_free(void *addr);

u16_t slab_chunk_size(void *addr);

void slab_recycle();

void cache_alloc_create(struct slabCache *cache, size_t size);

void *cache_alloc(struct slabCache *cache);

void cache_free(struct slabCache *cache, void *addr);

#define LIST_SIZE 10

// slab 管理的对象大小为 2 的幂,最小块为 4
// 以 8 为步长,每次块大小增加 8 ,内存碎片会更少,但写起来麻烦
#define SLAB_MAX (2<<LIST_SIZE)
#define SLAB_MIN 4

#ifdef TEST

void test_slab_alloc();

void test_slab_recycle();

#endif //TEST

#endif //QUARKOS_SLAB_H
