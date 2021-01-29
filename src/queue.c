//
// Created by pjs on 2021/1/29.
//

#include "queue.h"

void queueU8_append(void *self, uint8_t value) {

}

uint8_t queueU8_get(void *self) {

}

void queueU8_destroy(void *self) {
//    free()
}

queueU8_t QueueU8() {
    //TODO 需要 malloc!!!
    queueU8_t queue = {
            .size = QUEUE_LEN,
            .tail = 0,
            .header = 0,
            .full = false,
            .empty = true,
            .get = queueU8_get,
            .append = queueU8_append,
            .destroy = queueU8_destroy
    };
    return queue;
}