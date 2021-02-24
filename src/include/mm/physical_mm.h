//
// Created by pjs on 2021/2/1.
//

#ifndef QUARKOS_PHYSICAL_MM_H
#define QUARKOS_PHYSICAL_MM_H

#include "types.h"

#define PAGE_NULL 0xffffffff

void phymm_init();

pointer_t phymm_alloc();

void phymm_free(pointer_t addr);

void test_physical_mm();

#endif //QUARKOS_PHYSICAL_MM_H
