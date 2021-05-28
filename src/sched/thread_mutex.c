//
// Created by pjs on 2021/4/8.
//

#include "sched/thread_mutex.h"
#include "sched/kthread.h"


void thread_mutex_init(thread_mutex_t *lock) {
    spinlock_init(&lock->lock);
    list_header_init(&lock->sleep);
}

void thread_mutex_lock(thread_mutex_t *lock) {
    spinlock_lock(&lock->lock);
    while (lock->locked) {
        block_thread(&lock->sleep, &lock->lock);
    }
    lock->locked = true;
    spinlock_unlock(&lock->lock);
};

void thread_mutex_unlock(thread_mutex_t *lock) {
    spinlock_lock(&lock->lock);
    assertk(lock->locked);
    lock->locked = false;
    if (!list_empty(&lock->sleep)) {
        unblock_thread(&lock->sleep);
    }
    spinlock_unlock(&lock->lock);
}
