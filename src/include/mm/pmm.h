//
// Created by pjs on 2021/2/1.
//

#ifndef QUARKOS_MM_PMM_H
#define QUARKOS_MM_PMM_H

#include "types.h"

#define PMM_NLL 0xffffffff

void phymm_init();

pointer_t phymm_alloc();

void phymm_free(pointer_t addr);

#ifdef TEST
void test_physical_mm();
#endif //TEST

#endif //QUARKOS_MM_PMM_H
