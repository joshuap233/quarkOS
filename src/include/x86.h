//
// Created by pjs on 2021/1/25.
//
#ifndef QUARKOS_X86_H
#define QUARKOS_X86_H

#include "types.h"

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

// 端口 port 数据读取到内存 addr 处(4*cnt 字节)
// intel x86 手册只能找到 insd 指令(d - 双字)
__attribute__((always_inline))
static inline void insl(int port, void *addr, int cnt) {
    asm volatile(
    "cld\n\t"
    "rep insl" :
    "=D" (addr), "=c" (cnt) :
    "d" (port), "0" (addr), "1" (cnt) :
    "memory", "cc");
}

__attribute__((always_inline))
static inline void outsl(int port, const void *addr, int cnt) {
    asm volatile(
    "cld\n\t"
    " rep outsl" :
    "=S" (addr), "=c" (cnt) :
    "d" (port), "0" (addr), "1" (cnt) :
    "cc");
}


__attribute__((always_inline))
static inline void io_wait(void) {
    // 等待一个 io 读写的时间
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
static inline void pause() {
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


__attribute__((always_inline))
static inline uint32_t cupid_available() {
#define CPUID_MASK (0b1 << 21)
    uint32_t eflags = get_eflags() | CPUID_MASK;
    set_eflags(eflags);
    return get_eflags() & CPUID_MASK;
}

__attribute__((always_inline))
static inline uint32_t cpu_core() {
    uint32_t reg;
    asm volatile("cpuid":"=a"(reg):"a"(4));
    return ((reg >> 26) & 0x3f) + 1;
}

#define INTERRUPT_MASK (0b1 << 9)

// gcc 优化屏障
#define opt_barrier() asm volatile("": : :"memory")

#endif //QUARKOS_X86_H
