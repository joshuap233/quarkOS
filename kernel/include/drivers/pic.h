//
// Created by pjs on 2021/1/26.
//
//8259 pic
#ifndef QUARKOS_DRIVERS_PIC_H
#define QUARKOS_DRIVERS_PIC_H

#include "x86.h"
#define PIC1_CMD    0x20
#define PIC2_CMD    0xa0
#define PIC1_DAT    0x21
#define PIC2_DAT    0xa1
#define PIC_EOI     0x20

extern uint8_t pic2_offset;

// 通知 pic1 结束中断
INLINE void pic1_eoi(){
    outb(PIC1_CMD, PIC_EOI);
}

// 通知 PIC 结束中断
INLINE void pic2_eoi(uint8_t pic_offset) {
    if (pic_offset >= pic2_offset)
        outb(PIC2_CMD, PIC_EOI);
    // 当需要通知 pic2 结束时,同时也需要通知 pic1
    outb(PIC1_CMD, PIC_EOI);
}

#endif //QUARKOS_DRIVERS_PIC_H
