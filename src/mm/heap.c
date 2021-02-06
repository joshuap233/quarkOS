//
// Created by pjs on 2021/2/3.
//
// 内核堆

#include "heap.h"
#include "types.h"
#include "mm.h"
#include "virtual_mm.h"
#include "link_list.h"


//追踪空闲空间
static list_t heap;

void heap_init() {
    heap.size = PAGE_SIZE;
    heap.addr = (pointer_t) vmm_alloc(1);//初始内核堆为大小为1页
    heap.next = MM_NULL;
}

void *mallocK(size_t size) {
    heap_chunk_t *chunk = list_split_ff(&heap, CHUNK_SIZE(size));
    chunk->size = size;
    chunk->magic = HEAP_MAGIC;
    //TODO: 扩展堆
    return chunk == MM_NULL ? MM_NULL : chunk->addr;
}

pointer_t *freeK(void *addr) {
    heap_chunk_t *chunk = addr;
    assertk(chunk->magic == HEAP_MAGIC);
    list_free(&heap, (pointer_t) chunk->addr, chunk->size);
}


//TODO:扩展堆区域


