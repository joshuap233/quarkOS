//
// Created by pjs on 2021/2/22.
//

#ifndef QUARKOS_KLOCK_H
#define QUARKOS_KLOCK_H

#include "x86.h"

//使用这种方式会导致中断丢失,即无论调用 kLock 前是否开启中断
//调用 kRelease 后都会开启中断
__attribute__((always_inline))
static inline void k_lock(){
    disable_interrupt();
}

__attribute__((always_inline))
static inline void k_unlock(){
    enable_interrupt();
}


//需要在中断开启后初始化该锁
void irq_lock_init();

void irq_lock();

void irq_unlock();

// 自旋锁不能保证公平性,即每个线程都能进入临界区
// 可用于临界区较短的代码
typedef struct spinlock {
    uint32_t flag;
} spinlock_t;

__attribute__((always_inline))
static inline void spinlock_init(spinlock_t *lock) {
    lock->flag = 0;
}

extern void spinlock_lock(spinlock_t *lock);

__attribute__((always_inline))
static inline void spinlock_unlock(spinlock_t *lock) {
    lock->flag = 0;
}

//tick lock 能保证所有线程都能抢到锁
typedef struct tick_lock {
    uint32_t ticket;
    uint32_t turn;
} ticklock_t;

extern uint32_t fetch_and_add(uint32_t *ptr);

static inline void ticklock_init(ticklock_t *lock) {
    lock->ticket = 0;
    lock->turn = 0;
}

static inline void ticklock_lock(ticklock_t *lock) {
    while (lock->turn != fetch_and_add(&lock->ticket)) {
        pause();
    }
}

static inline void ticklock_unlock(ticklock_t *lock) {
    fetch_and_add(&lock->turn);
}

#endif //QUARKOS_KLOCK_H
