//
// Created by pjs on 2021/2/22.
//

#ifndef QUARKOS_KLOCK_H
#define QUARKOS_KLOCK_H

#include "x86.h"

//使用这种方式会导致中断丢失,即无论调用 kLock 前是否开启中断
//调用 kRelease 后都会开启中断
extern void k_lock();

extern void k_unlock();


void lock_init();

void irq_lock();

void irq_unlock();


typedef struct spin_lock {
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

#endif //QUARKOS_KLOCK_H
