//
// Created by pjs on 2021/1/25.
//
// 常用内联汇编
#ifndef QUARKOS_X86_H
#define QUARKOS_X86_H

#include "types.h"

// 读一个字节
static inline uint8_t inb(uint16_t port) {
    uint8_t res;
    asm volatile ("inb %1,%0":"=a"(res):"dN"(port));
    return res;
}

//读一个字
static inline uint16_t inw(uint16_t port) {
    uint16_t res;
    asm volatile ("inw %1,%0":"=a"(res):"dN"(port));
    return res;
}

// 写一个字节
static inline void outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %1,%0"::"dN"(port), "a"(value));
}

//写一个字
static inline void outw(uint16_t port, uint16_t value) {
    asm volatile ("outw %1,%0"::"dN"(port), "a"(value));
}


static inline void io_wait(void) {
    // 让 CPU 等待 IO 完成,IRQ 还没有初始化时可以使用这个
    asm volatile ( "outb %%al, $0x80" : : "a"(0));
}

// 设置 if 位
static inline void enable_interrupt() {
    asm volatile ("sti");
}

static inline void disable_interrupt() {
    asm volatile ("cli");
}


static inline void halt() {
    asm volatile ("hlt");
}


// 内核异常,停止运行
static inline void panic() {
    disable_interrupt();
    halt();
}

#endif //QUARKOS_X86_H
