//
// Created by pjs on 2021/2/6.
//

#ifndef QUARKOS_LINK_LIST_H
#define QUARKOS_LINK_LIST_H

#include "types.h"

// 管理空闲空间链表
typedef struct free_list {
    pointer_t addr;     //空闲空间起始地址
    pointer_t end_addr;
    uint64_t size;     //缓存一份大小
    struct free_list *next;
} list_t;

bool list_split(list_t *list, pointer_t va, uint32_t size);

// 使用首次适应查找,返回空闲页虚拟地址
void *list_split_ff(list_t *list, uint32_t size);

void list_free(list_t *list, pointer_t va, uint32_t size);

//合并连续空闲空间
void list_merge(list_t *list);

#endif //QUARKOS_LINK_LIST_H
