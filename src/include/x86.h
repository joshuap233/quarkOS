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
// 注意,如果需要将 enable/disable 当成锁一样使用
// 比如 disable_interrupt(); do_something...; enable_interrupt();
// 需要保存/还原 eflags,因为调用 enable/disable 前可能中断已经被禁用
// 或者使用 counter 计算中断被禁用次数,当被禁用次数为0 时再启用中断
static inline void enable_interrupt() {
    asm volatile ("sti");
}

static inline void disable_interrupt() {
    asm volatile ("cli");
}


static inline void halt() {
    asm volatile ("hlt");
}


static inline void enable_paging() {
    asm volatile (
    "mov %%cr0, %%eax       \n\t"
    "or  $0x80000000, %%eax \n\t"
    "mov %%eax, %%cr0"
    :: :"%eax");
}


// 内核异常,停止运行
static inline void panic() {
    disable_interrupt();
    halt();
}

static inline bool is_paging() {
    uint32_t cr0;
    asm volatile("mov %%cr0, %0":"=a"(cr0));
    return (cr0 >> 31) == 1;
}

//获取产生错误的虚拟地址
static inline pointer_t pf_addr() {
    uint32_t cr2;
    asm volatile("mov %%cr2, %0":"=a"(cr2));
    return cr2;
}


#endif //QUARKOS_X86_H
