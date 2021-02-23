//
// Created by pjs on 2021/1/26.
//
#include "types.h"
#include "x86.h"
#include "drivers/timer.h"
#include "klib/qlib.h"
#include "klib/qmath.h"
#include "sched/klock.h"

// 时钟中断
void pit_init(uint32_t frequency) {
    // frequency 为输出频率(HZ)
    // divisor 为计数器初始值
    uint32_t divisor = PIT_OSC_FREQUENCY / frequency;

    outb(PIT_CMD, PIT_CHANNEL0 | PIT_LH_MODE | PIT_MODE3 | PIT_BINARY_MODE);

    // 分别写入高字节和低字节
    outb(PIT_C0_DAT, (uint8_t) (divisor & MASK_U32(8)));
    outb(PIT_C0_DAT, (uint8_t) (divisor >> 8));
}

volatile timer_t g_timer[TIMER_COUNT] = {0};

__attribute__((always_inline))
static inline volatile mseconds_t *find_free_timer() {
    for (int i = 0; i < TIMER_COUNT; ++i) {
        if (g_timer[i].countdown == 0)
            return &g_timer[i].countdown;
    }
    return NULL;
}


bool ssleep(mseconds_t ms) {
/*
 * 睡眠时间（毫秒）为 10 的整数倍,ms<10 取 10
 */
    volatile mseconds_t *countdown;
    spinlock_t lock;
    spinlock_init(&lock);

    spinlock_lock(&lock);
    if ((countdown = find_free_timer()) == NULL)
        return false;
    *countdown = DIV_CEIL(ms, PIT_TIME_SLICE);
    spinlock_unlock(&lock);

    while (*countdown > 0)
        halt();
    return true;
}


// rtc 实时时钟
//void rtc_init(){
//
//}