//
// Created by pjs on 2021/4/8.
//

#ifndef QUARKOS_TASK_SLEEPLOCK_H
#define QUARKOS_TASK_SLEEPLOCK_H

#include "types.h"
#include "lib/list.h"
#include "lib/spinlock.h"

typedef struct thread_mutex {
    bool locked;        // 该锁是否已经被持有
    spinlock_t lock;    // 保证该锁的数据修改原子性
    list_head_t sleep;   // 该锁的休眠线程队列
} thread_mutex_t;

void thread_mutex_init(thread_mutex_t *lock);
void thread_mutex_lock(thread_mutex_t *lock);
void thread_mutex_unlock(thread_mutex_t *lock);

#endif //QUARKOS_TASK_SLEEPLOCK_H
