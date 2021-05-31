//
// Created by pjs on 2021/5/5.
//

#ifndef QUARKOS_MM_MEMBLOCK_H
#define QUARKOS_MM_MEMBLOCK_H

#include <types.h>
#include <lib/list.h>

ptr_t block_alloc(u32_t size);

ptr_t block_alloc_align(u32_t size, u32_t align);

u32_t block_size();

void block_set_g_mem_start();

ptr_t block_low_mem_size();

ptr_t block_high_mem_size();

struct block_allocator {
    list_head_t head;
    ptr_t addr;  // 管理器起始地址, <= addr 部分内存需要直接映射
    u32_t total;
};

typedef struct blockInfo {
    list_head_t head;
    u32_t size; // 块大小,包括该块头
} blockInfo_t;

extern struct block_allocator blockAllocator;
extern ptr_t g_mem_start;

#define block_mem_entry(ptr) list_entry(ptr, blockInfo_t, head)

#endif //QUARKOS_MM_MEMBLOCK_H
