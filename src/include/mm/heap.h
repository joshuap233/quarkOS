//
// Created by pjs on 2021/2/3.
//

#ifndef QUARKOS_HEAP_H
#define QUARKOS_HEAP_H

#include "types.h"
#include "klib/list.h"
void *mallocK(size_t size);

void freeK(void *addr);

void heap_init();

// 堆内存块头块结构
typedef struct heap_chunk_ptr {
    list_head_t head;
    uint32_t size; //当前内存块长度,包括头块
    uint32_t used: 1;  //当前内存块是否被使用
    uint32_t magic: 31;
#define HEAP_MAGIC 0x35e92b2e
} heap_ptr_t;

typedef struct heap {
    list_head_t header; //header 指向空头块
    uint32_t size;      //已分配虚拟内存大小
#define HEAP_START           0x20000000
#define HEAP_END             0x2fffffff
#define HEAP_SIZE            (HEAP_END - HEAP_START + 1)
#define HEAP_FREE_LIMIT      (2.5 * PAGE_SIZE)
//堆最后一个空闲块空闲空间大于 LIMIT 时,释放多余的页
} heap_t;

void *allocK_page();

// =============== 测试 =================
void test_mallocK_and_freeK();

void test_shrink_and_expand();
void test_allocK_page();
#endif //QUARKOS_HEAP_H
