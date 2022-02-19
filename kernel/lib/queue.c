//
// Created by pjs on 2021/5/9.
//
#include <lib/queue.h>
#include <lib/spinlock.h>

void lfQueue_node_init(lfq_node *node) {
    node->next = NULL;
}


// dummy 为空节点,不包含数据
void lfQueue_init(lf_queue *queue) {
    lfq_node *dummy = &queue->dummy;
    dummy->next = NULL;
    queue->head = dummy;
    queue->tail = dummy;
    spinlock_init(&queue->lock);
}

void lfQueue_put(lf_queue *queue, lfq_node *node) {
    spinlock_lock(&queue->lock);

    if (queue->head->next == NULL) {
        queue->head->next = node;
    } else {
        queue->tail->next = node;
    }
    node->next = NULL;
    queue->tail = node;

    spinlock_unlock(&queue->lock);
}

// 出队
lfq_node *lfQueue_get(lf_queue *queue) {
    spinlock_lock(&queue->lock);

    lfq_node *next;
    next = queue->head->next;
    if (next == NULL){
        spinlock_unlock(&queue->lock);
        return NULL;
    }
    queue->head->next = next->next;

    spinlock_unlock(&queue->lock);
    return next;
}

