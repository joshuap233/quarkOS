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

void slab_free(void *addr);
#define SLAB_MAX (PAGE_SIZE - sizeof(slabInfo_t))


#endif //QUARKOS_SLAB_H
