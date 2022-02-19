//
// Created by pjs on 2021/5/10.
//

#ifndef QUARKOS_LIB_SPINLOCK_H
#define QUARKOS_LIB_SPINLOCK_H

#include <types.h>
#include <x86.h>
#include <lib/qstring.h>
#include <lib/qlib.h>
#include <lib/irlock.h>

extern uint32_t test_and_set(uint32_t *flag);

typedef struct spinlock {
#define SPINLOCK_LOCKED 1
#define SPINLOCK_UNLOCK 0
    uint32_t flag;

#ifdef DEBUG
    struct cpu *cpu; // 持有锁的 cpu
    char *name; // 持有锁的函数名
#endif // DEBUG
} spinlock_t;

INLINE bool spinlock_locked(spinlock_t *lock) {
    return lock->flag == SPINLOCK_LOCKED;
}


INLINE void spinlock_init(spinlock_t *lock) {
    lock->flag = SPINLOCK_UNLOCK;
}


INLINE void spinlock_unlock(spinlock_t *lock) {
#ifdef DEBUG
    struct cpu *cpu = getCpu();
    cpu->lock = NULL;
    lock->cpu = NULL;
    lock->name  = NULL;
#endif
    lock->flag = 0;
}


INLINE void spinlock_lock(spinlock_t *lock) {
    // TODO:
    // 自旋锁需要关闭中断, 否则会造成死锁:
    // 线程 A 使用资源 a -> 调度程序切换为线程 B,
    // B 正在进行中断处理且使用资源 a, 那么中断程序中的自旋锁会死锁
    while (test_and_set(&lock->flag) == SPINLOCK_LOCKED) {
        pause();
    }

#ifdef DEBUG
    struct cpu *cpu = getCpu();
    cpu->lock = lock;
    lock->cpu = cpu;
    lock->name  = func_name(1);
#endif
    opt_barrier();
}


#endif //QUARKOS_LIB_SPINLOCK_H
