#ifndef QUARKOS_IDT_H
#define QUARKOS_IDT_H

#include "types.h"


void idt_init();

typedef struct interrupt_frame {
    uint32_t eip;
    uint16_t cs;
    uint32_t flags: 20;
    uint32_t zero: 12;
    // 特权级切换时会入栈
    uint16_t esp;
    uint16_t ss;
}__attribute__((packed)) interrupt_frame_t;


typedef struct error_code {
    uint16_t ext: 1;
    uint16_t idt: 1;
    uint16_t ti: 1;
    uint16_t segment_selector_index: 13;
    uint16_t zero;
}__attribute__((packed)) error_code_t;

void register_isr(uint8_t n,void *isr);


#endif //QUARKOS_IDT_H
