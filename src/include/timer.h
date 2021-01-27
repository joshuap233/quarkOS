//
// Created by pjs on 2021/1/26.
//
// 8253/8253 PIT 以及 RTC
#ifndef QUARKOS_TIMER_H
#define QUARKOS_TIMER_H

// 8 bit 端口
#define PIT_C0_DAT 0x40
#define PIT_C2_DAT 0x42
#define PIT_CMD    0x43
#define PIT_OSC_FREQUENCY   1193182
#define PIT_TIMER_FREQUENCY 200
#define PIT_CHANNEL0        (0b00  << 6)
#define PIT_LH_MODE         (0b11  << 4)
#define PIT_MODE3           (0b011 << 1)
// 模式2,3都会自动复位
#define PIT_BINARY_MODE     0b0

void pit_init(uint32_t frequency);

#endif //QUARKOS_TIMER_H
