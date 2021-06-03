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

u32_t block_start();

u32_t block_end();

typedef struct blockInfo {
    list_head_t head;
    u32_t addr;
    u32_t size; // 块大小,包括该块头
} blockInfo_t;

typedef blockInfo_t block_allocator_t;

extern block_allocator_t blockAllocator;

extern ptr_t g_mem_start;

#define block_mem_entry(ptr) list_entry(ptr, blockInfo_t, head)

#endif //QUARKOS_MM_MEMBLOCK_H
