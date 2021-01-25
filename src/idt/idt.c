//
// Created by pjs on 2021/1/23.
//

#include "qstdint.h"

#include "idt.h"
#include "qlib.h"



// 设置 interrupt gate
static void idt_set_ig(idt_t *idt,uint32_t offset,uint16_t selector,uint8_t type){
    interrupt_gate_t *ig = &(idt ->interrupt_gate);
    ig->zero = 0;
    ig->type = type;
    ig->selector = selector;
    ig->offset_h = offset >> 16;
    ig->offset_l = offset & MASK_U32(16);
}

void idt_init() {
    static idt_t _Alignas(8) idt[IDT_COUNT] = {0};
    idtr_t idtr = {
            .limit = IDT_SIZE * IDT_COUNT - 1,
            .address = (uint32_t) idt
    };
    //暂时全部使用 interrupt gate(调用 ISR 时自动清除 IF 位)
    idt_set_ig(&idt[0],(uint32_t)(ISR(0)),IDT_SC,IDT_TYPE);
    idt_set_ig(&idt[1],(uint32_t)(ISR(1)),IDT_SC,IDT_TYPE);
    idt_set_ig(&idt[2],(uint32_t)(ISR(2)),IDT_SC,IDT_TYPE);
    idt_set_ig(&idt[3],(uint32_t)(ISR(3)),IDT_SC,IDT_TYPE);
    idt_set_ig(&idt[4],(uint32_t)(ISR(4)),IDT_SC,IDT_TYPE);
    idt_set_ig(&idt[5],(uint32_t)(ISR(5)),IDT_SC,IDT_TYPE);
    idt_set_ig(&idt[6],(uint32_t)(ISR(6)),IDT_SC,IDT_TYPE);
    idt_set_ig(&idt[7],(uint32_t)(ISR(7)),IDT_SC,IDT_TYPE);
    idt_set_ig(&idt[8],(uint32_t)(ISR(8)),IDT_SC,IDT_TYPE);
    idt_set_ig(&idt[9],(uint32_t)(ISR(9)),IDT_SC,IDT_TYPE);
    idt_set_ig(&idt[10],(uint32_t)(ISR(10)),IDT_SC,IDT_TYPE);
    idt_set_ig(&idt[11],(uint32_t)(ISR(11)),IDT_SC,IDT_TYPE);
    idt_set_ig(&idt[12],(uint32_t)(ISR(12)),IDT_SC,IDT_TYPE);
    idt_set_ig(&idt[13],(uint32_t)(ISR(13)),IDT_SC,IDT_TYPE);
    idt_set_ig(&idt[14],(uint32_t)(ISR(14)),IDT_SC,IDT_TYPE);
    idt_set_ig(&idt[16],(uint32_t)(ISR(16)),IDT_SC,IDT_TYPE);
    idt_set_ig(&idt[17],(uint32_t)(ISR(17)),IDT_SC,IDT_TYPE);
    idt_set_ig(&idt[18],(uint32_t)(ISR(18)),IDT_SC,IDT_TYPE);
    idt_set_ig(&idt[19],(uint32_t)(ISR(19)),IDT_SC,IDT_TYPE);
    idt_set_ig(&idt[20],(uint32_t)(ISR(20)),IDT_SC,IDT_TYPE);

    idtr_set((uint32_t)&idtr);
}