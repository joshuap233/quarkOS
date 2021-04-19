//
// Created by pjs on 2021/4/8.
//

#include "sched/sleeplock.h"
#include "sched/kthread.h"


void sleeplock_init(sleeplock_t *lock) {
    spinlock_init(&lock->lock);
    list_header_init(&lock->head);
}

void sleeplock_lock(sleeplock_t *lock) {
    spinlock_lock(&lock->lock);
    if (!lock->locked) {
        lock->locked = true;
    } else {
        block_thread(&lock->head, &lock->lock);
    }
    spinlock_unlock(&lock->lock);
};

void sleeplock_unlock(sleeplock_t *lock) {
    spinlock_lock(&lock->lock);
    if (!lock->locked) {
        lock->locked = false;
    }
    spinlock_unlock(&lock->lock);
}