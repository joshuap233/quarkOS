//
// Created by pjs on 2021/1/26.
//
#include "types.h"
#include "x86.h"
#include "timer.h"
#include "qlib.h"
#include "qmath.h"

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

//每个时钟中断加一
static volatile uint32_t tick = 0;

void increment_tick() {
    tick++;
}

void ssleep(mseconds_t ms) {
/*
 * 操作系统睡眠
 * 不精确的睡眠函数，睡眠时间（毫秒）为 10 的整数倍,ms<10 取 10
 */
    tick = 0;
    uint32_t t = divUc(ms, 1000 / PIT_TIMER_FREQUENCY);
    while (tick < t) {
        halt();
    }
}


// rtc 实时时钟
//void rtc_init(){
//
//}