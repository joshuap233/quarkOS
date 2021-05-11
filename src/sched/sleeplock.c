//
// Created by pjs on 2021/4/8.
//

#include "sched/sleeplock.h"
#include "sched/kthread.h"


void sleeplock_init(sleeplock_t *lock) {
    spinlock_init(&lock->lock);
    list_header_init(&lock->sleep);
}

void sleeplock_lock(sleeplock_t *lock) {
    spinlock_lock(&lock->lock);
    if (!lock->locked) {
        lock->locked = true;
    } else {
        block_thread(&lock->sleep, NULL);
    }
    spinlock_unlock(&lock->lock);
};

void sleeplock_unlock(sleeplock_t *lock) {
    spinlock_lock(&lock->lock);
    assertk(lock->locked);
    lock->locked = false;
    if (!list_empty(&lock->sleep)) {
        unblock_thread(&lock->sleep);
    }
    spinlock_unlock(&lock->lock);
}
