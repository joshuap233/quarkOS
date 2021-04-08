//
// Created by pjs on 2021/2/22.
//

#ifndef QUARKOS_SCHED_KLOCK_H
#define QUARKOS_SCHED_KLOCK_H

#include "x86.h"

extern uint32_t fetch_and_add(uint32_t *ptr);

extern uint32_t test_and_set(uint32_t *flag);

typedef struct interrupt_lock {
    uint32_t ir_enable;
} ir_lock_t;


INLINE void ir_lock(ir_lock_t *lock) {
    lock->ir_enable = get_eflags() & INTERRUPT_MASK;
    // 只会禁用当前线程中断
    if (lock->ir_enable)
        disable_interrupt();
}

INLINE void ir_unlock(ir_lock_t *lock) {
    if (lock->ir_enable)
        enable_interrupt();
}


// 自旋锁不能保证公平性,即每个线程都能进入临界区
// 可用于临界区较短的代码
typedef struct spinlock {
    uint32_t flag;
} spinlock_t;

INLINE void spinlock_init(spinlock_t *lock) {
    lock->flag = 0;
}

INLINE void spinlock_lock(spinlock_t *lock) {
    while (test_and_set(&lock->flag) == 1)
        pause();
    opt_barrier();
}

INLINE void spinlock_unlock(spinlock_t *lock) {
    lock->flag = 0;
}

//tick lock 能保证所有线程都能抢到锁
typedef struct tick_lock {
    uint32_t ticket;
    uint32_t turn;
} ticklock_t;


INLINE void ticklock_init(ticklock_t *lock) {
    lock->ticket = 0;
    lock->turn = 0;
}

INLINE void ticklock_lock(ticklock_t *lock) {
    while (lock->turn != fetch_and_add(&lock->ticket)) {
        pause();
    }
}

INLINE void ticklock_unlock(ticklock_t *lock) {
    fetch_and_add(&lock->turn);
}

#endif //QUARKOS_SCHED_KLOCK_H
