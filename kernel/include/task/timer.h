//
// Created by pjs on 2021/2/23.
//

#ifndef QUARKOS_TASK_TIMER_H
#define QUARKOS_TASK_TIMER_H

#include <types.h>
#include <lib/list.h>



bool ms_sleep(mseconds_t msc);

bool ms_sleep_until(uint64_t msc);

void thread_timer_init();

extern volatile uint64_t g_tick;

#define G_TIME_SINCE_BOOT g_tick // 单位为 ms

#endif //QUARKOS_TASK_TIMER_H
