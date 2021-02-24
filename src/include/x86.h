//
// Created by pjs on 2021/1/25.
//
#ifndef QUARKOS_X86_H
#define QUARKOS_X86_H

#include "types.h"
#include "klib/qlib.h"

// 读一个字节
__attribute__((always_inline))
static inline uint8_t inb(uint16_t port) {
    uint8_t res;
    asm volatile ("inb %1,%0":"=a"(res):"dN"(port));
    return res;
}

//读一个字
__attribute__((always_inline))
static inline uint16_t inw(uint16_t port) {
    uint16_t res;
    asm volatile ("inw %1,%0":"=a"(res):"dN"(port));
    return res;
}

// 写一个字节
__attribute__((always_inline))
static inline void outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %1,%0"::"dN"(port), "a"(value));
}

//写一个字
__attribute__((always_inline))
static inline void outw(uint16_t port, uint16_t value) {
    asm volatile ("outw %1,%0"::"dN"(port), "a"(value));
}

__attribute__((always_inline))
static inline void io_wait(void) {
    // 让 CPU 等待 IO 完成,IRQ 还没有初始化时可以使用这个
    asm volatile ( "outb %%al, $0x80" : : "a"(0));
}

// 设置 if 位
__attribute__((always_inline))
static inline void enable_interrupt() {
    asm volatile ("sti");
}

__attribute__((always_inline))
static inline void disable_interrupt() {
    asm volatile ("cli");
}

__attribute__((always_inline))
static inline void halt() {
    asm volatile ("hlt");
}

__attribute__((always_inline))
static inline void pause(){
    //停止固定的指令周期
    asm volatile ("pause");
}

__attribute__((always_inline))
static inline uint32_t get_eflags() {
    uint32_t eflags;
    asm volatile (
    "pushf\n\t"
    "popl %0"
    :"=r"(eflags)
    ::"memory"
    );
    return eflags;
}

__attribute__((always_inline))
static inline void set_eflags(uint32_t eflags) {
    asm volatile (
    "pushl %0\n\t"
    "popf"
    ::"rm"(eflags)
    :"memory"
    );
}

__attribute__((always_inline))
static inline void enable_paging() {
    asm volatile (
    "mov %%cr0, %%eax       \n\t"
    "or  $0x80000000, %%eax \n\t"
    "mov %%eax, %%cr0"
    :: :"%eax");
}




__attribute__((always_inline))
static inline bool is_paging() {
    uint32_t cr0;
    asm volatile("mov %%cr0, %0":"=a"(cr0));
    return (cr0 >> 31) == 1;
}

//获取产生错误的虚拟地址
__attribute__((always_inline))
static inline pointer_t pf_addr() {
    uint32_t cr2;
    asm volatile("mov %%cr2, %0":"=a"(cr2));
    return cr2;
}


#define INTERRUPT_MASK (0b1 << 9)
#endif //QUARKOS_X86_H
