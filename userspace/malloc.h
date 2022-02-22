//
// Created by pjs on 2022/2/22.
//

#ifndef QUARKOS_USERSPACE_MALLOC_H
#define QUARKOS_USERSPACE_MALLOC_H

void *malloc(size_t size);
void free(void *addr);
void test_malloc();
#endif //QUARKOS_USERSPACE_MALLOC_H
