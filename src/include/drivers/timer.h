//
// Created by pjs on 2021/1/26.
//
// 8253/8253 PIT 以及 RTC
#ifndef QUARKOS_DRIVERS_TIMER_H
#define QUARKOS_DRIVERS_TIMER_H

// 8 bit 端口,分别为 c0 c2 数据端口以及命令端口
#define PIT_C0_DAT 0x40
#define PIT_C2_DAT 0x42
#define PIT_CMD    0x43
#define PIT_OSC_FREQUENCY   1193182
#define PIT_TIMER_FREQUENCY 100  //10ms 中断一次
#define PIT_TIME_SLICE      (1000/PIT_TIMER_FREQUENCY)
#define PIT_CHANNEL0        (0b00  << 6)
#define PIT_LH_MODE         (0b11  << 4)
#define PIT_MODE3           (0b011 << 1)
// 模式2,3都会自动复位
#define PIT_BINARY_MODE     0b0

void pit_init(uint32_t frequency);

extern volatile uint64_t g_tick;
#define G_TIME_SINCE_BOOT g_tick // 单位为 10ms

#endif //QUARKOS_DRIVERS_TIMER_H
