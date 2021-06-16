//
// Created by pjs on 2021/5/10.
//

#ifndef QUARKOS_LIB_SPINLOCK_H
#define QUARKOS_LIB_SPINLOCK_H

#include <types.h>
#include <x86.h>

#ifndef SMP

#include <lib/irlock.h>

#endif //SMP

extern uint32_t test_and_set(uint32_t *flag);


#ifdef SMP

typedef struct spinlock {
#define SPINLOCK_LOCKED 1
#define SPINLOCK_UNLOCK 0
    uint32_t flag;
} spinlock_t;

INLINE bool spinlock_locked(spinlock_t *lock) {
    return lock->flag  == SPINLOCK_LOCKED;
}


INLINE void spinlock_init(spinlock_t *lock) {
    lock->flag = SPINLOCK_UNLOCK;
}


INLINE void spinlock_unlock(spinlock_t *lock) {
    lock->flag = 0;
}

INLINE bool spinlock_trylock(spinlock_t *lock) {
    return test_and_set(&lock->flag) != SPINLOCK_LOCKED;
}


INLINE void spinlock_lock(spinlock_t *lock) {
    while (test_and_set(&lock->flag) == SPINLOCK_LOCKED) {
        pause();
    }
    opt_barrier();
}


#else

typedef ir_lock_t spinlock_t;

INLINE bool spinlock_locked(spinlock_t *lock) {
    return lock->ir_enable & INTERRUPT_MASK;
}


INLINE void spinlock_init(UNUSED spinlock_t *lock) {
}


INLINE void spinlock_unlock(spinlock_t *lock) {
    ir_unlock(lock);
}

INLINE bool spinlock_trylock(spinlock_t *lock) {
    if (lock->ir_enable & INTERRUPT_MASK) {
        ir_lock(lock);
        return true;
    }
    return false;
}


INLINE void spinlock_lock(spinlock_t *lock) {
    ir_lock(lock);
}

#endif //SMP


#endif //QUARKOS_LIB_SPINLOCK_H
