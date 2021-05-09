//
// Created by pjs on 2021/5/9.
//
// https://coolshell.cn/articles/8239.html
// lock-free queue
#include "lib/queue.h"


#define compare_and_swap __sync_bool_compare_and_swap

// dummy 为空节点,不包含数据
void lfQueue_init(lf_queue *queue, lfq_node *dummy) {
    dummy->next = NULL;
    queue->head = dummy;
    queue->tail = dummy;
}

void lfq_node_init(lfq_node *node) {
    node->next = NULL;
}

// 入队
void lfQueue_put(lf_queue *queue, lfq_node *node) {
    node->next = NULL;
    lfq_node *tail;
    do {
        tail = queue->tail;
    } while (!compare_and_swap(&tail->next, NULL, node));

    compare_and_swap(&queue->tail, tail, node);
}


// 出队
lfq_node *lfQueue_get(lf_queue *queue) {
    lfq_node *head;
    do {
        head = queue->head;
        if (head->next == NULL)
            return NULL;
    } while (!compare_and_swap(&queue->head, head, head->next));
    return head->next;
}

