//
// Created by pjs on 2021/5/10.
//
// 关闭中断
#ifndef QUARKOS_LIB_IRLOCK_H
#define QUARKOS_LIB_IRLOCK_H

#include <types.h>
#include <cpu.h>
#include <drivers/mp.h>

INLINE void ir_lock() {
    struct cpu *cpu = getCpu();
    cpu->ir_enable = get_eflags() & INTERRUPT_MASK;

    if (cpu->ir_enable)
        disable_interrupt();
    opt_barrier();
}

INLINE void ir_unlock() {
    struct cpu *cpu = getCpu();
    if (cpu->ir_enable)
        enable_interrupt();
}

#endif //QUARKOS_LIB_IRLOCK_H
