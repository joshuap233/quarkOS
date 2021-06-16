//
// Created by pjs on 2021/2/22.
//
// 公平锁,能保证所有线程都能抢到锁
#ifndef QUARKOS_LIB_TICKLOCK_H
#define QUARKOS_LIB_TICKLOCK_H

extern uint32_t fetch_and_add(uint32_t *ptr);

typedef struct tick_lock {
    uint32_t ticket;
    uint32_t turn;
} ticklock_t;


INLINE void ticklock_init(ticklock_t *lock) {
    lock->ticket = 0;
    lock->turn = 0;
}

INLINE void ticklock_lock(ticklock_t *lock) {
    while (lock->turn != fetch_and_add(&lock->ticket)) {
        pause();
    }
}

INLINE void ticklock_unlock(ticklock_t *lock) {
    fetch_and_add(&lock->turn);
}

#endif //QUARKOS_LIB_TICKLOCK_H
