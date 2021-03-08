//
// Created by pjs on 2021/2/23.
//

#ifndef QUARKOS_SCHED_TIMER_H
#define QUARKOS_SCHED_TIMER_H

#include "types.h"
#include "kthread.h"

#define TIMER_COUNT 20
typedef struct timer {
    struct timer *next, *prev;
    volatile uint64_t time;
    tcb_t *thread;
} timer_t;


bool ms_sleep(mseconds_t msc);

bool ms_sleep_until(uint64_t msc);

void timer_handle();

void thread_timer_init();

extern volatile uint64_t time_slice;
#define TIME_SLICE_LENGTH 10  // 时间片长度为 100ms

#endif //QUARKOS_SCHED_TIMER_H
