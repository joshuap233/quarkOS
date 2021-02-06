//
// Created by pjs on 2021/2/3.
//

#ifndef QUARKOS_HEAP_H
#define QUARKOS_HEAP_H

#include <stddef.h>
#include "types.h"

void *mallocK(size_t size);

pointer_t *freeK(void *addr);
void heap_init();

typedef struct heap_chunk {
    void *addr;
    uint32_t size;
    uint32_t magic;
#define HEAP_MAGIC 0xadf1ba //用于检查堆块完整性
#define CHUNK_SIZE(size) ((size) + 2*sizeof(uint32_t))
}__attribute__((packed)) heap_chunk_t;

#endif //QUARKOS_HEAP_H
