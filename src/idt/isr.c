//
// Created by pjs on 2021/1/25.
//
#include "idt.h"
#include "pic.h"
#include "qlib.h"
#include "ps2.h"
#include "keyboard.h"
#include "timer.h"
#include "mm.h"
#include "kthread.h"

// 忽略 -Wunused-parameter
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include "isr.h"

// 编译器会保留 EFLAGS 外的所有寄存器,并使用 iret 返回
__attribute__((interrupt))
void ISR(0)(interrupt_frame_t *frame) {
    panic();

//    printfk("xxx\n");
}

__attribute__((interrupt))
void ISR(1)(interrupt_frame_t *frame) {
    panic();

}

__attribute__((interrupt))
void ISR(2)(interrupt_frame_t *frame) {
    panic();

}

__attribute__((interrupt))
void ISR(3)(interrupt_frame_t *frame) {
    panic();

}

__attribute__((interrupt))
void ISR(4)(interrupt_frame_t *frame) {

}

__attribute__((interrupt))
void ISR(5)(interrupt_frame_t *frame) {

}

__attribute__((interrupt))
void ISR(6)(interrupt_frame_t *frame) {

}

__attribute__((interrupt))
void ISR(7)(interrupt_frame_t *frame) {

}

__attribute__((interrupt))
void ISR(8)(interrupt_frame_t *frame, uint32_t error_code) {
    error_code_t *ec = (error_code_t *) (&error_code);
}

__attribute__((interrupt))
void ISR(9)(interrupt_frame_t *frame) {

}

__attribute__((interrupt))
void ISR(10)(interrupt_frame_t *frame, uint32_t error_code) {

}

__attribute__((interrupt))
void ISR(11)(interrupt_frame_t *frame, uint32_t error_code) {

}

__attribute__((interrupt))
void ISR(12)(interrupt_frame_t *frame, uint32_t error_code) {

}

__attribute__((interrupt))
void ISR(13)(interrupt_frame_t *frame, uint32_t error_code) {
    error_code_t *ec = (error_code_t *) (&error_code);
    assertk(ec->zero == 0);
    printfk("GP exception\n");
    printfk("gp error code: %u\n", error_code);
    panic();
}

__attribute__((interrupt))
void ISR(14)(interrupt_frame_t *frame, uint32_t error_code) {
//页错误
    pf_error_code_t *ec = (pf_error_code_t *) (&error_code);
    assertk(ec->zero == 0);
    assertk(ec->zero1 == 0);
    printfk("page fault, error code: %x\n", error_code);
    printfk("page fault addr: %x\n", pf_addr());
    panic();
}


__attribute__((interrupt))
void ISR(16)(interrupt_frame_t *frame) {

}

__attribute__((interrupt))
void ISR(17)(interrupt_frame_t *frame, uint32_t error_code) {

}

__attribute__((interrupt))
void ISR(18)(interrupt_frame_t *frame) {

}

__attribute__((interrupt))
void ISR(19)(interrupt_frame_t *frame) {

}

__attribute__((interrupt))
void ISR(20)(interrupt_frame_t *frame) {

}

// PIC 0 号中断,PIT 时钟中断
__attribute__((interrupt))
void ISR(32)(interrupt_frame_t *frame) {
    tick++;
    pic1_eoi();
    schedule();
}

// PIC 1 号中断,键盘输入
__attribute__((interrupt))
void ISR(33)(interrupt_frame_t *frame) {
    kb_sc_parse(ps2_rd());
    pic1_eoi();
}

#pragma GCC diagnostic pop
