//
// Created by pjs on 2021/2/1.
//

#ifndef QUARKOS_PHYSICAL_MM_H
#define QUARKOS_PHYSICAL_MM_H

#include "types.h"

//mem_size 为可用内存总大小
void phymm_init();

pointer_t phymm_alloc();

void phymm_free(pointer_t addr);


#endif //QUARKOS_PHYSICAL_MM_H
