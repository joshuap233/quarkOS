//
// Created by pjs on 2021/2/23.
//

#ifndef QUARKOS_TASK_TIMER_H
#define QUARKOS_TASK_TIMER_H

#include <types.h>
#include <lib/list.h>

#define TIMER_COUNT 20
typedef struct timer {
    list_head_t head;         //timer 列表
    list_head_t *thread;      //睡眠线程
    volatile uint64_t time;   //睡眠到 time 唤醒线程
} timer_t;


bool ms_sleep(mseconds_t msc);

bool ms_sleep_until(uint64_t msc);

void thread_timer_init();

extern volatile uint64_t g_tick;

#define G_TIME_SINCE_BOOT g_tick // 单位为 ms

#endif //QUARKOS_TASK_TIMER_H
