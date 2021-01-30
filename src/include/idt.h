#ifndef QUARKOS_IDT_H
#define QUARKOS_IDT_H

#include "types.h"

typedef struct idtr {
    uint16_t limit;
    uint32_t address;
}__attribute__((packed)) idtr_t;


// gate 描述符
struct gate_desc {
    uint16_t offset_l; // 处理程序入口偏移 0-15位
    uint16_t selector; // 目标代码段的段选择子
    uint8_t zero;      // 保留
    uint8_t type;
    uint16_t offset_h; // 偏移 16-31 位
}__attribute__((packed));

typedef struct gate_desc interrupt_gate_t;
typedef struct gate_desc trap_gate_t;

typedef union idt {
    interrupt_gate_t interrupt_gate;
    trap_gate_t trap_gate;
#define IDT_SIZE sizeof(union idt)
#define IDT_COUNT 256
#define IDT_TYPE 0b10001110 //interrupt_gate type 32 位,特权级为 0
#define IDT_INT_USR_TYPE32 0b11101110
#define IDT_SC  0x08     //代码段选择子
}__attribute__((packed)) idt_t;

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
    uint16_t ssi: 13;
    uint16_t zero;
}__attribute__((packed)) error_code_t;


extern void idtr_set(uint32_t idtr);

#endif //QUARKOS_IDT_H
