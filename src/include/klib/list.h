//
// Created by pjs on 2021/2/23.
//
// 双向循环链表,链表头节点数据域始终为空
#ifndef QUARKOS_LIST_H
#define QUARKOS_LIST_H

#include "types.h"

//双向链表指针域
typedef struct link_list_ptr {
    struct link_list_ptr *next, *prev;
} list_ptr_t;

#define list_header list_ptr_t

__attribute__((always_inline))
static inline void list_header_init(list_header *header) {
    header->next = NULL;
    header->prev = NULL;
}

__attribute__((always_inline))
static inline bool list_empty(list_header *header) {
    return header->next == NULL;
}

__attribute__((always_inline))
static inline void list_add(list_ptr_t *new, list_ptr_t *prev, list_ptr_t *next) {
    new->prev = prev;
    new->next = next;
    next->prev = new;
    prev->next = new;
}

__attribute__((always_inline))
static inline void list_add_before(list_ptr_t *new, list_ptr_t *target) {
    list_add(new, target->prev, target);
}

__attribute__((always_inline))
static inline void list_add_tail(list_ptr_t *new, list_ptr_t *target) {
    list_add(new, target, target->next);
}

__attribute__((always_inline))
static inline void list_del(list_ptr_t *list) {
    list->prev->next = list->next;
}


#endif //QUARKOS_LIST_H
