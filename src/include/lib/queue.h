//
// Created by pjs on 2021/5/9.
//
// 无锁队列

#ifndef QUARKOS_QUEUE_H
#define QUARKOS_QUEUE_H

#include "types.h"
#include "lib/qlib.h"

#define queue_entry container_of


typedef struct lf_queue_node {
    struct lf_queue_node *next;
} lfq_node;

typedef struct lf_queue {
    lfq_node *head;
    lfq_node *tail;
} lf_queue;

void lfQueue_put(lf_queue *queue, lfq_node *node);

void lfQueue_init(lf_queue *queue, lfq_node *dummy);

lfq_node *lfQueue_get(lf_queue *queue);

void lfq_node_init(lfq_node *node);

#endif //QUARKOS_QUEUE_H
