//
// Created by pjs on 2021/5/4.
//

#ifndef QUARKOS_SLAB_H
#define QUARKOS_SLAB_H

#include "types.h"
#include "mm/mm.h"
#include "lib/list.h"


typedef struct chunkLink {
    struct chunkLink *next;
} chunkLink_t;

typedef struct slabInfo {
    list_head_t head;         // 连接 slabInfo
    chunkLink_t *chunk;       // 指向第一个可用内存块
    uint16_t n_allocated;     // 已经分配块个数
    uint16_t size;            // 内存块大小
    uint32_t magic;
} slabInfo_t;

void *slab_alloc(uint16_t size);

u32_t fixSize(u32_t size);

void slab_free(void *addr);

void slab_recycle();

#define IS_POWER_OF_2(x) (!((x)&((x)-1)))
#define LIST_SIZE 8

// slab 管理的对象大小为 2 的幂,最小块为 4
// 以 8 为步长,每次块大小增加 8 ,内存碎片会更少,但写起来麻烦
#define SLAB_MAX (2<<LIST_SIZE)
#define SLAB_MIN 4

#ifdef TEST

void test_slab_alloc();
void test_slab_recycle();

#endif //TEST

#endif //QUARKOS_SLAB_H
