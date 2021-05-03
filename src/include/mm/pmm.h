//
// Created by pjs on 2021/2/1.
//

#ifndef QUARKOS_MM_PMM_H
#define QUARKOS_MM_PMM_H

#include "types.h"

#define PMM_NULL 0xffffffff

void pm_init();

ptr_t pm_alloc();

void pm_free(ptr_t addr);

#ifdef TEST
void test_physical_mm();
#endif //TEST

#endif //QUARKOS_MM_PMM_H
