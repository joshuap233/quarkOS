//
// Created by pjs on 2021/2/3.
//

#ifndef QUARKOS_HEAP_H
#define QUARKOS_HEAP_H

#include "types.h"

void *mallocK(size_t size);

void freeK(void *addr);

void heap_init();

// 堆内存块头块结构
// 修改结构后可能需要添加 __attribute__((packed)) 属性
typedef struct heap_chunk_ptr {
    struct heap_chunk_ptr *next, *prev;
    uint32_t size; //当前内存块长度,包括头块
    uint8_t magic: 7;
    uint8_t used: 1;  //当前内存块是否被使用
#define HEAP_MAGIC 0x76
}__attribute__((packed)) heap_ptr_t;

typedef struct heap {
    heap_ptr_t *header; //header指向第一个头块
    uint32_t size;      //已分配虚拟内存大小
#define HEAP_START           0x20000000
#define HEAP_END             0x2fffffff
#define HEAP_SIZE            (HEAP_END - HEAP_START + 1)
#define HEAP_FREE_LIMIT      (2.5 * PAGE_SIZE)
//堆最后一个空闲块空闲空间大于 LIMIT 时,释放多余的页
} heap_t;


// =============== 测试 =================
void test_mallocK_and_freeK();

void test_shrink_and_expand();

#endif //QUARKOS_HEAP_H
