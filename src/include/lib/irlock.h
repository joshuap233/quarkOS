//
// Created by pjs on 2021/5/10.
//
// 关闭中断
#ifndef QUARKOS_LIB_IRLOCK_H
#define QUARKOS_LIB_IRLOCK_H

#include "types.h"

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
#endif //QUARKOS_LIB_IRLOCK_H
