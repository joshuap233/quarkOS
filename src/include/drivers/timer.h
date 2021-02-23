//
// Created by pjs on 2021/1/26.
//
// 8253/8253 PIT 以及 RTC
#ifndef QUARKOS_TIMER_H
#define QUARKOS_TIMER_H

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
#define TIMER_COUNT         10 // 计时器数量

void pit_init(uint32_t frequency);

bool ssleep(mseconds_t ms);

typedef struct timer {
    volatile mseconds_t countdown;
} timer_t;
extern volatile timer_t g_timer[TIMER_COUNT];

__attribute__((always_inline))
static inline void timer_handle() {
    for (int i = 0; i < TIMER_COUNT; ++i) {
        if (g_timer[i].countdown > 0)
            g_timer[i].countdown--;
    }
}


#endif //QUARKOS_TIMER_H
