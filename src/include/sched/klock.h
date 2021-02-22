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

extern uint32_t irq_disable_counter;

static inline void irq_lock() {
    disable_interrupt();
    irq_disable_counter++;
}

static inline void irq_unlock() {
    irq_disable_counter--;
    if (irq_disable_counter == 0) {
        enable_interrupt();
    }
}

void lock_init();

extern int test_and_set(uint32_t *old_ptr, uint32_t new);


typedef struct spin_lock {
    uint32_t flag;
} spinlock_t;

void spinlock_init(spinlock_t *lock);

void spinlock_lock(spinlock_t *lock);

void spinlock_unlock(spinlock_t *lock);

#endif //QUARKOS_KLOCK_H
