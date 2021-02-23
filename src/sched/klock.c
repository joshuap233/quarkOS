//
// Created by pjs on 2021/2/22.
//

#include "sched/klock.h"
#include "klib/qlib.h"
#include "x86.h"

uint32_t irq_disable_counter;


void irq_lock_init() {
    assertk(get_eflags() & INTERRUPT_MASK);
    irq_disable_counter = 0;
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

//void futex_wait(){
//
//}
//
//void mutex_lock(int *mutex) {
//
//}
//
//void mutex_unlock(int *mutex) {
//
//}



