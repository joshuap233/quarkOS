//
// Created by pjs on 2021/2/1.
//

#ifndef QUARKOS_VIRTUAL_MM_H
#define QUARKOS_VIRTUAL_MM_H

#include "types.h"

extern void cr3_set(pointer_t);

void vmm_init();

bool mm_map(pointer_t va, pointer_t pa, uint32_t size);
void *vmm_alloc(uint32_t size);
#endif //QUARKOS_VIRTUAL_MM_H
