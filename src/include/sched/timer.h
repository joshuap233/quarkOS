//
// Created by pjs on 2021/2/23.
//

#ifndef QUARKOS_SCHED_TIMER_H
#define QUARKOS_SCHED_TIMER_H

#include "types.h"
#include "lib/list.h"

#define TIMER_COUNT 20
typedef struct timer {
    list_head_t head;         //timer 列表
    list_head_t *thread;      //睡眠线程
    volatile uint64_t time;   //睡眠到 time 唤醒线程
} timer_t;


bool ms_sleep(mseconds_t msc);

bool ms_sleep_until(uint64_t msc);

void thread_timer_init();

extern volatile uint64_t g_time_slice;
#define TIME_SLICE_LENGTH 10  // 时间片长度为 100ms

#endif //QUARKOS_SCHED_TIMER_H
