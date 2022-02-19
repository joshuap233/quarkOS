//
// Created by pjs on 2022/2/10.
//
#include <types.h>
#include <drivers/mp.h>
#include <lib/qlib.h>
#include <isr.h>
#include <cpu.h>

// 寄存器索引
#define IOAPIC_ID      0
#define IOAPIC_VER     1
#define IO_TBL         0x10      // 64 位寄存器

#define IR_CNT_MAST    MASK_U32(8)
#define INT_DISABLED   (1<<16)  // Interrupt disabled

static u32_t volatile *ioapic;

static void ioapicw(u32_t idx, u32_t data) {
    // 使用 IOREGSEL 寄存器选择要使用的寄存器
    ioapic[0] = idx;

    // 向 IOREGWIN 寄存器写数据
    ioapic[4] = data;
}

u32_t ioapicr(u32_t idx) {
    ioapic[0] = idx;
    return ioapic[4];
}

void ioapic_init() {
    assertk(cpuCfg.ioapicPtr != 0);
    ioapic = (u32_t *) cpuCfg.ioapicPtr;

    u32_t irqCnt = (ioapicr(IOAPIC_VER) >> 16) & IR_CNT_MAST;
    assertk(irqCnt!=0);
    assertk((ioapicr(IOAPIC_ID) >> 24) == cpuCfg.ioApicId);

    // 关闭所有中断,将中断设置为高电平边缘触发(isa irq 为高电平触发)
    // Destination Mode	为 Physical Destination
    for (u32_t i = 0; i < irqCnt; i++) {
        ioapicw(IO_TBL + 2 * i, INT_DISABLED | (IRQ0 + i));
        ioapicw(IO_TBL + 2 * i + 1, 0);
    }
}

void irqEnable(int irq, int16_t lapicId) {
    // lapicId 为处理当当前中断的 lapicId ( 0 ~ (c_cpu-1) )
    static u16_t id = 0;
    if (lapicId == -1) {
        id = (id + 1) % cpuCfg.nCpu;
        lapicId = cpus[id].apic_id;
    }
    ioapicw(IO_TBL + 2 * irq, IRQ0 + irq);
    ioapicw(IO_TBL + 2 * irq + 1, lapicId << 24);
}
