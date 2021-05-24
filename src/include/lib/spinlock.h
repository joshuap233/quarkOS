//
// Created by pjs on 2021/5/10.
//

#ifndef QUARKOS_LIB_SPINLOCK_H
#define QUARKOS_LIB_SPINLOCK_H

#include <types.h>
#include <x86.h>

extern uint32_t test_and_set(uint32_t *flag);


// 自旋锁不能保证公平性,即每个线程都能进入临界区
// 可用于临界区较短的代码
typedef struct spinlock {
#define SPINLOCK_LOCKED 1
#define SPINLOCK_UNLOCK 0
    uint32_t flag;
} spinlock_t;

INLINE void spinlock_init(spinlock_t *lock) {
    lock->flag = SPINLOCK_UNLOCK;
}

INLINE void spinlock_lock(spinlock_t *lock) {
    while (test_and_set(&lock->flag) == SPINLOCK_LOCKED) {
        pause();
    }
    opt_barrier();
}

INLINE bool spinlock_trylock(spinlock_t *lock) {
    return test_and_set(&lock->flag) != SPINLOCK_LOCKED;
}

INLINE void spinlock_unlock(spinlock_t *lock) {
    lock->flag = 0;
}

#endif //QUARKOS_LIB_SPINLOCK_H
