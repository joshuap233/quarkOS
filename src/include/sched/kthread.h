//
// Created by pjs on 2021/2/18.
//
// 内核线程

#ifndef QUARKOS_SCHED_KTHREAD_H
#define QUARKOS_SCHED_KTHREAD_H

#include "types.h"
#include "lib/list.h"
#include "x86.h"
#include "mm/mm.h"
#include "lib/spinlock.h"
#include "sched/tcb.h"

int kthread_create(kthread_t *tid, void *(worker)(void *), void *args);

void kthread_set_time_slice(kthread_t tid, u16_t time_slice);

void kthread_exit();

int8_t block_thread(list_head_t *_block_list, spinlock_t *lock);

void unblock_thread(list_head_t *thread);

void kthread_set_name(kthread_t tid, const char *name);

list_head_t *kthread_get_run_list(kthread_t tid);

extern list_head_t *init_task;


_Noreturn INLINE void idle() {
    while (1) {
        halt();
    }
}

#endif //QUARKOS_SCHED_KTHREAD_H
