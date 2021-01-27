//
// Created by pjs on 2021/1/26.
//

#include "qstdint.h"
#include "x86.h"
#include "qlib.h"
#include "pic.h"
#include "timer.h"

// offset1: PIC 主片向量偏移
// offset2: PIC 从片向量偏移
// 0-0x1f 被 CPU 使用
void pic_init(uint32_t offset1, uint32_t offset2) {
    assertk(offset1 >= 0x20);
    assertk(offset2 >= 0x20);

    uint8_t m1, m2;
    m1 = inb(PIC1_DAT);
    m2 = inb(PIC2_DAT);

    outb(PIC1_CMD, ICW1_INIT);
    //老式机器 PIC 运行慢,所以需要等待写入
    io_wait();
    outb(PIC2_CMD, ICW1_INIT);
    io_wait();

    // 写于中断服务入口
    outb(PIC1_DAT, offset1);
    io_wait();
    outb(PIC2_DAT, offset2);
    io_wait();

    // 告诉主 PIC,从 PIC 连接到 IR2(0b0100)
    outb(PIC1_DAT, 4);
    io_wait();
    //告诉从 PIC,主 PIC 连接到 IR1(0b0010)
    outb(PIC2_DAT, 2);
    io_wait();

    //设置 PIC1,2 需要手动通知 PIC 结束中断
    outb(PIC1_DAT, ICW4_8086);
    io_wait();
    outb(PIC2_DAT, ICW4_8086);
    io_wait();

    outb(PIC1_DAT, m1);
    outb(PIC2_DAT, m2);

    enable_interrupt();
    pit_init(PIT_TIMER_FREQUENCY);
}

