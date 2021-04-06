//
// Created by pjs on 2021/1/23.
//

#include "types.h"

#include "idt.h"
#include "klib/qlib.h"
#include "drivers/pic.h"
#include "isr.h"
#include "drivers/ps2.h"

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

extern void idtr_set(uint32_t idtr);
static idt_t _Alignas(8) idt[IDT_COUNT] = {0};

// 设置 interrupt gate
static void idt_set_ig(idt_t *_idt, uint32_t offset, uint16_t selector, uint8_t type) {
    interrupt_gate_t *ig = &(_idt->interrupt_gate);
    ig->zero = 0;
    ig->type = type;
    ig->selector = selector;
    ig->offset_h = offset >> 16;
    ig->offset_l = offset & MASK_U32(16);
}

void register_isr(uint8_t n,void *isr){
    idt_set_ig(&idt[0], (pointer_t) isr, IDT_SC, IDT_TYPE);
}

void idt_init() {
    idtr_t idtr = {
            .limit = IDT_SIZE * IDT_COUNT - 1,
            .address = (pointer_t) idt
    };
    //暂时全部使用 interrupt gate(调用 ISR 时自动清除 IF 位)
    idt_set_ig(&idt[0], (pointer_t) (ISR(0)), IDT_SC, IDT_TYPE);
    idt_set_ig(&idt[1], (pointer_t) (ISR(1)), IDT_SC, IDT_TYPE);
    idt_set_ig(&idt[2], (pointer_t) (ISR(2)), IDT_SC, IDT_TYPE);
    idt_set_ig(&idt[3], (pointer_t) (ISR(3)), IDT_SC, IDT_TYPE);
    idt_set_ig(&idt[4], (pointer_t) (ISR(4)), IDT_SC, IDT_TYPE);
    idt_set_ig(&idt[5], (pointer_t) (ISR(5)), IDT_SC, IDT_TYPE);
    idt_set_ig(&idt[6], (pointer_t) (ISR(6)), IDT_SC, IDT_TYPE);
    idt_set_ig(&idt[7], (pointer_t) (ISR(7)), IDT_SC, IDT_TYPE);
    idt_set_ig(&idt[8], (pointer_t) (ISR(8)), IDT_SC, IDT_TYPE);
    idt_set_ig(&idt[9], (pointer_t) (ISR(9)), IDT_SC, IDT_TYPE);
    idt_set_ig(&idt[10], (pointer_t) (ISR(10)), IDT_SC, IDT_TYPE);
    idt_set_ig(&idt[11], (pointer_t) (ISR(11)), IDT_SC, IDT_TYPE);
    idt_set_ig(&idt[12], (pointer_t) (ISR(12)), IDT_SC, IDT_TYPE);
    idt_set_ig(&idt[13], (pointer_t) (ISR(13)), IDT_SC, IDT_TYPE);
    idt_set_ig(&idt[14], (pointer_t) (ISR(14)), IDT_SC, IDT_TYPE);
    idt_set_ig(&idt[16], (pointer_t) (ISR(16)), IDT_SC, IDT_TYPE);
    idt_set_ig(&idt[17], (pointer_t) (ISR(17)), IDT_SC, IDT_TYPE);
    idt_set_ig(&idt[18], (pointer_t) (ISR(18)), IDT_SC, IDT_TYPE);
    idt_set_ig(&idt[19], (pointer_t) (ISR(19)), IDT_SC, IDT_TYPE);
    idt_set_ig(&idt[20], (pointer_t) (ISR(20)), IDT_SC, IDT_TYPE);


    idt_set_ig(&idt[32], (pointer_t) (ISR(32)), IDT_SC, IDT_TYPE);
    idt_set_ig(&idt[33], (pointer_t) (ISR(33)), IDT_SC, IDT_TYPE);
    idt_set_ig(&idt[46], (pointer_t) (ISR(46)), IDT_SC, IDT_TYPE);

    idtr_set((pointer_t) &idtr);
    pic_init(32, 40);
    ps2_init();
}