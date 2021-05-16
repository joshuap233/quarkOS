//
// Created by pjs on 2021/2/1.
//

#ifndef QUARKOS_MM_PMM_H
#define QUARKOS_MM_PMM_H

#include "types.h"

#define PMM_NULL 0xffffffff

ptr_t pm_alloc(u32_t size);
ptr_t pm_alloc_page();

u32_t pm_free(ptr_t addr);
u32_t pm_chunk_size(ptr_t addr);

#ifdef TEST
void test_alloc();
#endif //TEST

#endif //QUARKOS_MM_PMM_H
