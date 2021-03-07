//
// Created by pjs on 2021/1/26.
//

#include "types.h"
#include "x86.h"
#include "klib/qlib.h"
#include "drivers/pic.h"
#include "drivers/timer.h"

__attribute__((always_inline))
static inline void outb_wait(uint16_t port, uint8_t value) {
    outb(port, value);
    // 等待 PIC 写入完成
    io_wait();
}


// offset1: PIC 主片向量偏移
// offset2: PIC 从片向量偏移
// 0-0x1f 被 CPU 使用
void pic_init(uint32_t offset1, uint32_t offset2) {
    assertk(offset1 >= 0x20);
    assertk(offset2 >= 0x20);

    uint8_t m1, m2;
    m1 = inb(PIC1_DAT);
    m2 = inb(PIC2_DAT);

    outb_wait(PIC1_CMD, ICW1_INIT);
    outb_wait(PIC2_CMD, ICW1_INIT);

    // 写于中断服务入口
    outb_wait(PIC1_DAT, offset1);
    outb_wait(PIC2_DAT, offset2);
    pic2_offset = offset2;

    // 告诉主 PIC,从 PIC 连接到 IR2(0b0100)
    outb_wait(PIC1_DAT, 4);
    //告诉从 PIC,主 PIC 连接到 IR1(0b0010)
    outb_wait(PIC2_DAT, 2);

    //设置 PIC1,2 需要手动通知 PIC 结束中断
    outb_wait(PIC1_DAT, ICW4_8086);
    outb_wait(PIC2_DAT, ICW4_8086);

    outb(PIC1_DAT, m1);
    outb(PIC2_DAT, m2);

    pit_init(PIT_TIMER_FREQUENCY);
}

