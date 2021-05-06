//
// Created by pjs on 2021/5/5.
//

#ifndef QUARKOS_MM_MEMBLOCK_H
#define QUARKOS_MM_MEMBLOCK_H

#include "types.h"
#include "lib/list.h"


typedef struct blockInfo {
    list_head_t head;
    u32_t size; // 块大小,包括该块头
} blockInfo_t;

struct block_allocator {
    list_head_t head;
    ptr_t addr;  // 管理器起始地址, <= addr 部分内存需要直接映射
    u32_t total;
};

extern struct block_allocator blkAllocator;

ptr_t block_alloc(u32_t size);

#endif //QUARKOS_MM_MEMBLOCK_H
