//
// Created by pjs on 2021/2/3.
//
// 内核堆

#include "heap.h"
#include "types.h"
#include "mm.h"
#include "virtual_mm.h"

//堆占用虚拟地址 0x1fffffff- 0x2fffffff
static heap_t heap;

//初始化一个新块
static void chunk_init(heap_ptr_t *c, heap_ptr_t *prev, uint32_t size) {
    c->next = MM_NULL;
    c->prev = prev;
    c->size = size;
    c->used = false;
    c->magic = HEAP_MAGIC;
}

void heap_init() {
    heap.size = PAGE_SIZE;
    heap.header = (heap_ptr_t *) HEAP_START;
    heap.tail = MM_NULL;
    heap.free = HEAP_SIZE - heap.size;
    chunk_init(heap.header, MM_NULL, PAGE_SIZE);
    vmm_mapv((pointer_t) heap.header, heap.size, VM_KW | VM_PRES);
}

// size 为还需要的内存块大小
static bool expend(uint32_t size) {
    size = SIZE_ALIGN(size);
    if (heap.free < size) return false;
    vmm_mapv(heap_tail(heap), size, VM_KW | VM_PRES);
    heap.free -= size;
    heap.size += size;
}

static void shrink() {
    pointer_t free = heap_tail(heap) - (pointer_t) heap.tail;
    if (free > LIMIT) {
        uint32_t size = free / PAGE_SIZE;
        vmm_unmap((void *) (heap_tail(heap) - size), size);
    }
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
        size += tail->size;
    }

    if (header != tail) {
        header->next = tail->next;
        header->size = size;
    }
    if (heap.header == MM_NULL || heap.header > header)
        heap.header = header;
    if (heap.tail == MM_NULL || (heap.tail < tail && heap.tail > header))
        heap.tail = alloc->prev;
}


void *mallocK(size_t size) {
    size = CHUNK_SIZE(size);
    if (heap.size < size) {
        if (!expend(heap.size - size))
            return MM_NULL;
    }
    heap_ptr_t *header = heap.header;
    while (header != MM_NULL) {
        if (!header->used && header->size >= size) {
            heap_ptr_t *alloc = header;

            if (header->size - size < sizeof(heap_ptr_t)) {
                //剩余空间无法容纳新头块,将剩余空间全都分配出去
                size = header->size;
                alloc->next = MM_NULL;
                header = MM_NULL;
            } else {
                header = (void *) header + size;
                //初始化下一个内存块
                chunk_init(header, (heap_ptr_t *) alloc, header->size - size);
                alloc->next = (heap_ptr_t *) header;
            }
            alloc->used = true;
            alloc->size = size;
            heap.header = header;
            heap.tail = alloc;
            return ALLOC_ADDR((void *) alloc);
        }
        header = header->next;
    }
    return MM_NULL;
}

void freeK(void *addr) {
    heap_ptr_t *alloc = CHUNK_HEADER(addr);
    assertk(alloc->magic = HEAP_MAGIC);
    alloc->used = false;
    merge(alloc);
    shrink();
}




