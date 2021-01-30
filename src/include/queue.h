//
// Created by pjs on 2021/1/29.
//

#ifndef QUARKOS_QUEUE_H
#define QUARKOS_QUEUE_H

#include "types.h"
#include "stddef.h"

#define QUEUE_LEN 10


void queueU8_destroy(void *self);

void queueU8_append(void *self, uint8_t value);


uint8_t queueU8_get(void *self); //返回首元素并删除


//typedef uint8_t (*GET)(size_t index);
//typedef void (*APPEND)(uint8_t value);
//typedef void (*DESTROY)();

typedef struct queueU8 {
    //TODO: 使用 uint8_t 指针,malloc 分配 queue
    uint8_t queue[QUEUE_LEN];
    size_t size;
    size_t header;
    size_t tail;
    bool empty;
    bool full;

    uint8_t (*get)(void *self);

    void (*append)(void *self, uint8_t value);

    void (*destroy)(void *self);
} queueU8_t;

queueU8_t QueueU8();

typedef union queue {
    queueU8_t qU8;
} queue_t;

#endif //QUARKOS_QUEUE_H
