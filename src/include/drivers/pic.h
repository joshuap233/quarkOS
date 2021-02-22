//
// Created by pjs on 2021/1/26.
//
//8259 pic
#ifndef QUARKOS_PIC_H
#define QUARKOS_PIC_H


#define PIC1_CMD    0x20
#define PIC2_CMD    0xa0
#define PIC1_DAT    0x21
#define PIC2_DAT    0xa1
#define PIC_EOI     0x20

#include "x86.h"
// initialize command word

#define ICW1_ICW4        0x1                //本次初始化需要发送 ICW4
#define ICW1_CASCADE    (0x0<<1)        //级联
#define ICW1_LTIM        (0x1<<4)        //边沿触发

#define ICW1_INIT       ICW1_ICW4 | ICW1_CASCADE | ICW1_LTIM
#define ICW4_8086        0x01           // 需要手动添加 eoi 结束中断(级联必须选择)


void pic_init(uint32_t offset1, uint32_t offset2);

static uint8_t pic2_offset = 40;

// 通知 pic1 结束中断
static inline void pic1_eoi(){
    outb(PIC1_CMD, PIC_EOI);
}

// 通知 PIC 结束中断
static inline void pic_eoi(uint8_t pic_offset) {
    if (pic_offset >= pic2_offset)
        outb(PIC2_CMD, PIC_EOI);
    // 当需要通知 pic2 结束时,同时也需要通知 pic1
    outb(PIC1_CMD, PIC_EOI);
}

#endif //QUARKOS_PIC_H
