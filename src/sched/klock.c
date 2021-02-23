//
// Created by pjs on 2021/2/22.
//

#include "sched/klock.h"
#include "klib/qlib.h"
#include "x86.h"

uint32_t irq_disable_counter;

__attribute__((always_inline))
static inline void irq_lock_init() {
    assertk(get_eflags() & INTERRUPT_MASK);
    irq_disable_counter = 0;
}

void lock_init() {
    irq_lock_init();
}


void irq_lock() {
    disable_interrupt();
    irq_disable_counter++;
}

void irq_unlock() {
    irq_disable_counter--;
    if (irq_disable_counter == 0) {
        enable_interrupt();
    }
}



