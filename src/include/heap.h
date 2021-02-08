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
typedef struct heap_ptr {
    struct heap_ptr *next;
    struct heap_ptr *prev;
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


#endif //QUARKOS_HEAP_H
