//
// Created by pjs on 2021/1/26.
//
#include "types.h"
#include "x86.h"
#include "drivers/timer.h"
#include "klib/qlib.h"
#include "klib/qmath.h"

volatile uint64_t g_tick;
//开机到当前时间的毫秒*10

// 时钟中断
void pit_init(uint32_t frequency) {
    // frequency 为输出频率(HZ)
    // divisor 为计数器初始值
    g_tick = 0;
    uint32_t divisor = PIT_OSC_FREQUENCY / frequency;

    outb(PIT_CMD, PIT_CHANNEL0 | PIT_LH_MODE | PIT_MODE3 | PIT_BINARY_MODE);

    // 分别写入高字节和低字节
    outb(PIT_C0_DAT, (uint8_t) (divisor & MASK_U32(8)));
    outb(PIT_C0_DAT, (uint8_t) (divisor >> 8));

}



// rtc 实时时钟
//void rtc_init(){
//
//}