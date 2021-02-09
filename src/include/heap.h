//
// Created by pjs on 2021/2/3.
//

#ifndef QUARKOS_HEAP_H
#define QUARKOS_HEAP_H

#include <stddef.h>
#include "types.h"

void *mallocK(size_t size);

void freeK(void *addr);

void heap_init();

// 堆内存块头块结构
typedef struct heap_chunk_ptr {
    struct heap_chunk_ptr *next;
    struct heap_chunk_ptr *prev;
    uint32_t size; //当前内存块长度,包括头块
    uint8_t magic: 7;
    uint8_t used: 1;  //当前内存块是否被使用
#define CHUNK_SIZE(size)   ((size) + sizeof(heap_ptr_t))
//size为需要分配的内存大小,返回包括头块的大小
#define CHUNK_HEADER(addr) ((addr) - sizeof(heap_ptr_t))
//addr 为需要释放的内存地址,返回包括头块的地址
#define ALLOC_ADDR(addr)   ((addr) + sizeof(heap_ptr_t)-1)
//计算实际分配的内存块首地址
#define HEAP_MAGIC 0x76
}__attribute__((packed)) heap_ptr_t;

typedef struct heap {
    heap_ptr_t *header; //header指向第一个空闲块(该空闲块后可能有非空闲块)
    heap_ptr_t *tail;   //tail 指向最后一个非空闲块
    uint32_t size;      //已分配虚拟内存
    uint32_t free;      //空闲虚拟内存
#define HEAP_START           0x20000000
#define HEAP_END             0x2fffffff
#define HEAP_SIZE            (HEAP_END - HEAP_START + 1)
#define HEAP_FREE_LIMIT      (2.5 * PAGE_SIZE)
//空闲堆空间大于 LIMIT 时,释放多余的页
} heap_t;

//返回当前堆末尾地址
static inline pointer_t heap_tail(heap_t heap) {
    return HEAP_SIZE + heap.size;
}

#endif //QUARKOS_HEAP_H
