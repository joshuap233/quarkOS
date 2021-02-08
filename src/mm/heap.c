//
// Created by pjs on 2021/2/3.
//
// 内核堆

#include "heap.h"
#include "types.h"
#include "mm.h"
#include "virtual_mm.h"


//chunk总是指向首个空闲块
static heap_ptr_t *chunk;


static void chunk_init(heap_ptr_t *c, heap_ptr_t *prev, uint32_t size) {
    c->next = MM_NULL;
    c->prev = prev;
    c->size = size;
    c->used = false;
    c->magic = HEAP_MAGIC;
}

void heap_init() {
    chunk = (heap_ptr_t *) vmm_alloc(PAGE_SIZE);
    chunk_init(chunk, MM_NULL, PAGE_SIZE);
}


//合并内存块,并将 chunk 指向首个空闲块
static void merge(heap_ptr_t *alloc) {
    heap_ptr_t *header = alloc, *tail = alloc;
    uint32_t size = alloc->size;
    //向前合并
    while (header->prev != MM_NULL && !header->prev->used) {
        header = header->prev;
        size += header->size;
    }
    //向后合并
    while (tail->next != MM_NULL && !tail->next->used) {
        tail = tail->next;
        size += header->size;
    }

    if (header != tail) {
        header->next = tail->next;
        header->size = size;
        if (chunk > header) chunk = header;
    }
}


void *mallocK(size_t size) {
    //TODO: 实现扩展堆
    size = CHUNK_SIZE(size);
    while (chunk->next != MM_NULL) {
        if (!chunk->used && chunk->size >= size) {
            heap_ptr_t *alloc = chunk;

            if (chunk->size - size < sizeof(heap_ptr_t)) {
                //剩余空间无法容纳新头块,将剩余空间全都分配出去
                size = chunk->size;
                alloc->next = MM_NULL;
            } else {
                chunk = (void *) chunk + size;
                //初始化下一个内存块
                chunk_init(chunk, (heap_ptr_t *) alloc, chunk->size - size);
                alloc->next = (heap_ptr_t *) chunk;
            }

            alloc->used = true;
            alloc->size = size;

            return ALLOC_ADDR((void *) alloc);
        }
    }
    return MM_NULL;
}

void freeK(void *addr) {
    heap_ptr_t *alloc = CHUNK_HEADER(addr);
    assertk(alloc->magic = HEAP_MAGIC);
    alloc->used = false;
    merge(alloc);
}




