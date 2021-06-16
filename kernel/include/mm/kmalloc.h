//
// Created by pjs on 2021/5/4.
//

#ifndef QUARKOS_KMALLOC_H
#define QUARKOS_KMALLOC_H

#include "types.h"

void *kmalloc(u32_t size);

void kfree(void *addr);

void *krealloc(void *_addr, size_t _size);

void *kcalloc(u32_t size);

#endif //QUARKOS_KMALLOC_H
