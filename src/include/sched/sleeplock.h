//
// Created by pjs on 2021/4/8.
//

#ifndef QUARKOS_SCHED_SLEEPLOCK_H
#define QUARKOS_SCHED_SLEEPLOCK_H

#include "types.h"
#include "lib/list.h"
#include "lib/spinlock.h"

typedef struct sleeplock {
    bool locked;        // 该锁是否已经被持有
    spinlock_t lock;    // 保证该锁的数据修改原子性
    list_head_t sleep;   // 该锁的休眠线程队列
} sleeplock_t;

void sleeplock_init(sleeplock_t *lock);
void sleeplock_lock(sleeplock_t *lock);
void sleeplock_unlock(sleeplock_t *lock);

#endif //QUARKOS_SCHED_SLEEPLOCK_H
