//
// Created by pjs on 2021/1/26.
//
//
#ifndef QUARKOS_DRIVERS_PIT_H
#define QUARKOS_DRIVERS_PIT_H

#define PIT_TIMER_FREQUENCY 100  // 10ms 中断一次

extern volatile uint64_t g_tick;
#define G_TIME_SINCE_BOOT g_tick // 单位为 ms

#endif //QUARKOS_DRIVERS_PIT_H
