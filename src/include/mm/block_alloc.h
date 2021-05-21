//
// Created by pjs on 2021/5/5.
//

#ifndef QUARKOS_MM_MEMBLOCK_H
#define QUARKOS_MM_MEMBLOCK_H

#include "types.h"
#include "lib/list.h"

ptr_t block_alloc(u32_t size);

ptr_t block_alloc_align(u32_t size, u32_t align);
u32_t block_size();
u32_t block_start();

#endif //QUARKOS_MM_MEMBLOCK_H
