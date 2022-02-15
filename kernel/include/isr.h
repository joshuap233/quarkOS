//
// Created by pjs on 2021/1/27.
//
//中断处理函数
#ifndef QUARKOS_ISR_H
#define QUARKOS_ISR_H

#include <types.h>

typedef struct interrupt_frame {
    uint32_t eip;
    uint32_t cs;
    uint32_t flags;
    // 特权级切换时会入栈
    uint32_t esp;
    uint32_t ss;
}PACKED interrupt_frame_t;


typedef struct error_code {
    uint16_t ext: 1;
    uint16_t idt: 1;
    uint16_t ti: 1;
    uint16_t segment_selector_index: 13;
    uint16_t zero;
}PACKED error_code_t;

#define ISR(index) isr ## index


void reg_isr(uint8_t n,void *isr);
void idt_init();
void load_idtr();

#define IRQ0             32

#define IRQ_TIMER        0
#define IRQ_KBD          1
#define IRQ_COM1         4
#define IRQ_IDE          14
#define IRQ_ERROR        19

#define IRQ_SPURIOUS     31

#endif //QUARKOS_ISR_H
