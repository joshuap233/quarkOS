//
// Created by pjs on 2022/2/10.
//
#include <types.h>
#include <drivers/mp.h>
#include <lib/qlib.h>
#include <isr.h>
#include <drivers/cmos.h>
#include <highmem.h>
#include <drivers/lapic.h>

#define APIC_ID 0x0020   // ID
#define VER     0x0030   // Version
#define TPR     0x0080   // Task Priority
#define EOI     0x00B0   // EOI
#define SVR     0x00F0   // Spurious interrupt vector
#define ICR_LO  0x0300   // Interrupt Command 低 32 位
#define ICR_HI  0x0310   // Interrupt Command 高 32 位
#define ESR     0x0280   // Error Status
#define TIMER   0x0320   // Local Vector Table 0 (TIMER)
#define PCINT   0x0340   // Performance Counter LVT
#define LINT0   0x0350   // Local Vector Table 1 (LINT0)
#define LINT1   0x0360   // Local Vector Table 2 (LINT1)
#define ERROR   0x0370   // Local Vector Table 3 (ERROR)
#define TICR    0x0380   // Timer Initial Count
#define TCCR    0x0390   // Timer Current Count
#define TDCR    0x03E0   // Timer Divide Configuration

#define STARTUP         (0b110 << 8) // Startup IPI
#define ASSERT          (1<<14)      // Assert interrupt (非 INIT Level De-assert 用)
#define DEASSERT        0            // INIT Level De-assert 用这个
#define ENABLE          (1<<8)       // APIC Enable
#define INIT            (0b101<<8)   // INIT Level De-assert
#define LEVEL           (1<<15)      // 水平触发
#define IPI_ALL         (0b10 << 18) // 向所有 CPU 发送消息(包括自己)
#define IPI_PENDING     (1<<12)      // 正在发送 IPI 消息
#define X1              0b1011       // 频率除 1
#define PERIODIC        (0b01<<17)   // apic timer Periodic 模式
#define MASKED          (1<<16)      // Interrupt masked


static u32_t volatile *lapic;
volatile uint64_t g_tick = 0;

INLINE void lapicW(int index, int value) {
    lapic[index / 4] = value;
    lapic[APIC_ID / 4];  // 等待寄存器写完
}

INLINE u32_t lapicR(int index) {
    return lapic[index / 4];
}

void lapic_init() {
    assertk(cpuCfg.lapicPtr != 0);
    lapic = (void *) cpuCfg.lapicPtr;

    // 开启 lapic, 设置 spurious 中断号
    lapicW(SVR, ENABLE | (IRQ0 + IRQ_SPURIOUS));

    // TODO: 设置时钟频率
    lapicW(TDCR, X1);
    lapicW(TIMER, PERIODIC | (IRQ0 + IRQ_TIMER));
    lapicW(TICR, 10000000);

    // 屏蔽 LINT(禁用 lapic 时, 可以把 LINT) 当 INTR 引脚用)
    lapicW(LINT0, MASKED);
    lapicW(LINT1, MASKED);

    // 禁用 performance counter
    if (((lapicR(VER) >> 16) & 0xFF) >= 4) {
        lapicW(PCINT, MASKED);
    }

    lapicW(ERROR, IRQ0 + IRQ_ERROR);

    // 读错误寄存器前需要写一次,清空寄存器
    lapicW(ESR, 0);
    lapicW(ESR, 0);

    lapicW(EOI, 0);//??

    // 必须先写 ICR_HI(IPI_ALL 设置这个没用)
    lapicW(ICR_HI, 0);
    //向所有 cpu 发送 cpu 间消息
    lapicW(ICR_LO, IPI_ALL | INIT | LEVEL);
    // 查看 ipi 是否完成
    while (lapicR(ICR_LO) & IPI_PENDING);

    // 设置优先级为 0, apic 接收所有中断
    lapicW(TPR, 0);
}

void microdelay(int us) {
}

// 启用 application processor
void lapicStartAp(u8_t apicid, u32_t addr) {
    int i;

    // shutdown status byte :
    // 0Ah: bios 会跳到 0x467 记录的入口地址处
    cmos_w(0xf, 0xa);
    u16_t *wrv = (u16_t *) (0x467 + HIGH_MEM);
    wrv[0] = 0;
    wrv[1] = addr >> 4;

    //TODO
    lapicW(ICR_HI, apicid << 24);
    lapicW(ICR_LO, INIT | LEVEL | ASSERT);
    microdelay(200);
    lapicW(ICR_LO, INIT | LEVEL);
    microdelay(100);

    for (i = 0; i < 2; i++) {
        lapicW(ICR_HI, apicid << 24);
        lapicW(ICR_LO, STARTUP | (addr >> 12));
        microdelay(200);
    }
}


u32_t lapicId() {
    return lapicR(APIC_ID) >> 24;
}

void lapicEoi() {
    lapicW(EOI, 0);
}