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
volatile uint32_t tick = 0;


void ssleep(mseconds_t ms) {
/*
 * 内核睡眠函数
 * 不精确的睡眠，睡眠时间（毫秒）为 10 的整数倍,ms<10 取 10
 * 不能在多线程中使用!!否则在获取 eflags 后切换线程可能导致 eflags 陈旧
 */
    uint32_t eflags = get_eflags();
    disable_interrupt();
    tick = 0;
    uint32_t end = DIV_CEIL(ms, 1000 / PIT_TIMER_FREQUENCY);
    set_eflags(eflags);
    while (tick < end)
        halt();
}


// rtc 实时时钟
//void rtc_init(){
//
//}