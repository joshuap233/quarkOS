//
// Created by pjs on 2021/2/3.
//
// 内核堆
//TODO: 测试!!!
// 线程安全?
#include "heap.h"
#include "types.h"
#include "mm.h"
#include "virtual_mm.h"

static heap_t heap;


//返回当前堆末尾地址
#define HEAP_TAIL (HEAP_START + heap.size - 1)

//计算剩余可用于扩展堆的虚拟内存
#define HEAP_VMM_FREE (HEAP_SIZE - heap.size)

static inline void *alloc_addr(void *addr) {
    //计算实际分配的内存块首地址
    return addr + sizeof(heap_ptr_t);
}

static inline void *chunk_header(void *addr) {
    //addr 为需要释放的内存地址,返回包括头块的地址
    return addr - sizeof(heap_ptr_t);
}

static inline size_t chunk_size(size_t size) {
    //size为需要分配的内存大小,返回包括头块的大小
    return size + sizeof(heap_ptr_t);
}


//初始化一个新块
static void chunk_init(heap_ptr_t *c, heap_ptr_t *prev, heap_ptr_t *next, uint32_t size) {
    c->next = next;
    c->prev = prev;
    c->size = size;
    c->used = false;
    c->magic = HEAP_MAGIC;
}

void heap_init() {
    heap.size = PAGE_SIZE;
    heap.header = (heap_ptr_t *) HEAP_START;
    heap.tail = heap.header;
    vmm_mapv((pointer_t) heap.header, heap.size, VM_KW | VM_PRES);
    chunk_init(heap.header, MM_NULL, MM_NULL, PAGE_SIZE);
}

// size 为需要扩展的内存块大小
static bool expend(uint32_t size) {
    size = SIZE_ALIGN(size);
    if (HEAP_VMM_FREE < size) return false;
    vmm_mapv(HEAP_TAIL + 1, size, VM_KW | VM_PRES);
    if (heap.tail->used) {
        heap_ptr_t *new = (heap_ptr_t *) (HEAP_TAIL + 1);
        chunk_init(new, heap.tail, MM_NULL, size);
        heap.tail = new;
    } else {
        heap.tail->size += size;
    }
    heap.size += size;
    return true;
}

static inline void shrink() {
    if (heap.tail->used) return;
    pointer_t free = heap.tail->size;
    if (free > HEAP_FREE_LIMIT) {
        pointer_t free_size = free / PAGE_SIZE;
        heap.tail->size -= free_size;
        vmm_unmap((void *) heap.tail + heap.tail->size, free_size);
    }
}

//合并内存块
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
    if (heap.tail == tail) heap.tail = header;
}

// chunk 为可用空闲块, size 为需要分割的大小
static void *alloc_chunk(heap_ptr_t *chunk, size_t size) {
    heap_ptr_t *alloc = chunk;
    if (chunk->size - size < sizeof(heap_ptr_t)) {
        //剩余空间无法容纳新头块,将剩余空间全都分配出去
        size = chunk->size;
        alloc->next = MM_NULL;
    } else {
        heap_ptr_t *new = (void *) chunk + size;
        //初始化下一个内存块
        chunk_init(new, alloc, alloc->next, chunk->size - size);
        alloc->next = new;
    }
    alloc->used = true;
    alloc->size = size;
    if (heap.tail == alloc && alloc->next != MM_NULL)
        heap.tail = alloc->next;
    return alloc_addr(alloc);
}

void *mallocK(size_t size) {
    size = chunk_size(size);
    for (heap_ptr_t *header = heap.header; header != MM_NULL; header = header->next)
        if (!header->used && header->size >= size)
            return alloc_chunk(header, size);
    if (!expend(size)) return MM_NULL;
    return alloc_chunk(heap.tail, size);
}


void freeK(void *addr) {
    heap_ptr_t *alloc = chunk_header(addr);
    assertk(alloc->magic = HEAP_MAGIC);
    alloc->used = false;
    merge(alloc);
    shrink();
}
