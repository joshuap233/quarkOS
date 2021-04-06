//
// Created by pjs on 2021/1/26.
//
#include "types.h"
#include "x86.h"
#include "drivers/timer.h"
#include "klib/qlib.h"
#include "drivers/init.h"

// 8 bit 端口,分别为 c0 c2 数据端口以及命令端口
#define PIT_C0_DAT 0x40
#define PIT_C2_DAT 0x42
#define PIT_CMD    0x43
#define PIT_OSC_FREQUENCY   1193182
#define PIT_TIME_SLICE      (1000/PIT_TIMER_FREQUENCY)
#define PIT_CHANNEL0        (0b00  << 6)
#define PIT_LH_MODE         (0b11  << 4)
#define PIT_MODE3           (0b011 << 1)
// 模式2,3都会自动复位
#define PIT_BINARY_MODE     0b0

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