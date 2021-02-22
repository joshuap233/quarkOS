//
// Created by pjs on 2021/2/22.
//

#include "sched/klock.h"
#include "klib/qlib.h"
#include "x86.h"

// =============== irq_lock ========
uint32_t irq_disable_counter;

static inline void irq_lock_init() {
    assertk(get_eflags() & INTERRUPT_MASK);
    irq_disable_counter = 0;
}

void lock_init() {
    irq_lock_init();
}


void spinlock_init(spinlock_t *lock) {
    lock->flag = 0;
}


void spinlock_lock(spinlock_t *lock) {
    while (test_and_set(&lock->flag, 1) == 1) {
//        halt();
        pause();
    }
}

void spinlock_unlock(spinlock_t *lock) {
    lock->flag = 0;
}