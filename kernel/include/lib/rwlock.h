//
// Created by pjs on 2021/5/10.
//
// 读写锁
#ifndef QUARKOS_LIB_RWLOCK_H
#define QUARKOS_LIB_RWLOCK_H

#include <types.h>
#include <lib/spinlock.h>

typedef struct rwlock {
    spinlock_t lock;
    spinlock_t writelock;
    u16_t readers;      // 读者数量
} rwlock_t;

INLINE void rwlock_init(rwlock_t *lk) {
    lk->readers = 0;
    spinlock_init(&lk->lock);
    spinlock_init(&lk->writelock);
}

INLINE void rlock_lock(rwlock_t *lk) {
    spinlock_lock(&lk->lock);
    lk->readers++;
    if (lk->readers == 1) {
        spinlock_lock(&lk->writelock);
    }
    spinlock_unlock(&lk->lock);
}

INLINE void rlock_unlock(rwlock_t *lk) {
    spinlock_lock(&lk->lock);
    lk->readers--;
    if (lk->readers == 0) {
        spinlock_unlock(&lk->writelock);
    }
    spinlock_unlock(&lk->lock);
}

INLINE void wlock_lock(rwlock_t *lk) {
    spinlock_lock(&lk->writelock);
}

INLINE void wlock_unlock(rwlock_t *lk) {
    spinlock_unlock(&lk->writelock);
}

#endif //QUARKOS_LIB_RWLOCK_H
