//
// Created by pjs on 2021/2/23.
//
// 双向循环链表
#ifndef QUARKOS_LIB_LIST_H
#define QUARKOS_LIB_LIST_H

#include "types.h"

#define LIST_HEAD(name) \
    list_head_t name = { &(name), &(name) }

#define list_entry container_of


//双向链表指针域
typedef struct list_head {
    struct list_head *next, *prev;
} list_head_t;


INLINE void list_header_init(list_head_t *header) {
    header->next = header;
    header->prev = header;
}

INLINE void list_add(list_head_t *new, list_head_t *prev, list_head_t *next) {
    new->prev = prev;
    new->next = next;
    next->prev = new;
    prev->next = new;
}

INLINE void list_add_next(list_head_t *new, list_head_t *target) {
    list_add(new, target, target->next);
}

INLINE void list_add_prev(list_head_t *new, list_head_t *target) {
    list_add(new, target->prev, target);
}


INLINE void list_del(list_head_t *list) {
    list->prev->next = list->next;
    list->next->prev = list->prev;
}

INLINE bool list_empty(list_head_t *header) {
    return header->next == header;
}

INLINE void list_link(list_head_t *header, list_head_t *tail) {
    header->next = tail;
    tail->prev = header;
}

INLINE list_head_t *list_first(list_head_t *header) {
    return header->next;
}

INLINE list_head_t *list_tail(list_head_t *header) {
    return header->prev;
}

// 获取第一个元素并将该元素删除
INLINE list_head_t *list_get_first(list_head_t *header) {

    if (!list_empty(header)) {
        list_head_t *h = list_first(header);
        list_del(h);
        return h;
    }
    return NULL;
}


#define list_for_each(hdr, head) \
    for ((hdr) = (head)->next; (hdr) != (head); (hdr) = (hdr)->next)


#define list_for_each_del(hdr, nxt, head) \
        for ((hdr) = (head)->next, (nxt)=(hdr)->next; (hdr) != (head); (hdr) = (nxt),(nxt)=(nxt)->next)

// 逆序遍历
#define list_for_each_rev(hdr, head) \
    for ((hdr) = (head)->prev; (hdr) != (head); (hdr) = (hdr)->prev)


#define queue_empty list_empty
#define queue_head  list_first
#define queue_tail  list_tail
#define queue_get   list_get_first
#define queue_put   list_add_prev
#define queue_t     list_head_t
#define queue_init  list_header_init
#define QUEUE_HEAD  LIST_HEAD

#endif //QUARKOS_LIB_LIST_H
