//
// Created by pjs on 2021/2/23.
//
// 双向循环链表
#ifndef QUARKOS_LIST_H
#define QUARKOS_LIST_H

#include "types.h"

#define LIST_HEAD(name) \
    list_head_t name = { &(name), &(name) }

#define list_entry(ptr,type,member) \
    ((type *)((void *)(ptr) - offsetof(type,member)))


 //双向链表指针域
typedef struct list_head {
    struct list_head *next, *prev;
} list_head_t;


__attribute__((always_inline))
static inline void list_header_init(list_head_t *header) {
    header->next = header;
    header->prev = header;
}

__attribute__((always_inline))
static inline void list_add(list_head_t *new, list_head_t *prev, list_head_t *next) {
    new->prev = prev;
    new->next = next;
    next->prev = new;
    prev->next = new;
}

__attribute__((always_inline))
static inline void list_add_next(list_head_t *new, list_head_t *target) {
    list_add(new, target, target->next);
}

__attribute__((always_inline))
static inline void list_add_prev(list_head_t *new, list_head_t *target) {
    list_add(new, target->prev, target);
}


__attribute__((always_inline))
static inline void list_del(list_head_t *list) {
    list->prev->next = list->next;
    list->next->prev = list->prev;
}

__attribute__((always_inline))
static inline bool list_empty(list_head_t *header) {
    return header->next == header;
}

__attribute__((always_inline))
static inline void list_link(list_head_t *header, list_head_t *tail) {
    header->next = tail;
    tail->prev = header;
}

//#define list_foreach



#define list_for_each_del(head) \
    for (list_head_t *hdr = (head)->next, *next=hdr->next; hdr != (head); hdr = next,next=next->next)


#endif //QUARKOS_LIST_H
