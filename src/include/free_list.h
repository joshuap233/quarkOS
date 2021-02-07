//
// Created by pjs on 2021/2/6.
//

#ifndef QUARKOS_FREE_LIST_H
#define QUARKOS_FREE_LIST_H

#include "types.h"

// 管理空闲空间链表
typedef struct free_list {
    pointer_t addr;     //空闲空间起始地址
    uint32_t size;
    struct free_list *next, *prev;
} free_list_t;

//用于管理空闲列表
typedef struct stack {
#define LIST_COUNT 64
#define STACK_SIZE 128
    pointer_t list[STACK_SIZE];
    uint32_t top;
    uint32_t size;
} stack_t;

bool list_split(pointer_t va, uint32_t size);

// 使用首次适应查找,返回空闲页虚拟地址
void *list_split_ff(uint32_t size);

void list_free(pointer_t va, uint32_t size);


void free_list_init(uint32_t size);

#endif //QUARKOS_FREE_LIST_H
