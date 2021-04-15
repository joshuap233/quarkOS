//
// Created by pjs on 2021/1/25.
//
#ifndef QUARKOS_X86_H
#define QUARKOS_X86_H

#include "types.h"


// 端口 port 数据读取到内存 addr 处(2*cnt 字节)
INLINE void insw(int port, void *addr, int cnt) {
    asm volatile(
    "cld\n\t"
    "rep insw" :
    "=D" (addr), "=c" (cnt) :
    "d" (port), "0" (addr), "1" (cnt) :
    "memory", "cc");
}

INLINE void outsw(int port, const void *addr, int cnt) {
    asm volatile(
    "cld\n\t"
    "rep outsw" :
    "=S" (addr), "=c" (cnt) :
    "d" (port), "0" (addr), "1" (cnt) :
    "cc");
}

// 读一个字节
INLINE uint8_t inb(uint16_t port) {
    uint8_t res;
    asm volatile ("inb %1,%0":"=a"(res):"dN"(port));
    return res;
}

//读一个字
INLINE uint16_t inw(uint16_t port) {
    uint16_t res;
    asm volatile ("inw %1,%0":"=a"(res):"dN"(port));
    return res;
}


INLINE uint32_t inl(uint16_t port) {
    uint32_t res;
    asm volatile ("inl %1,%0":"=a"(res):"dN"(port));
    return res;
}

// 写一个字节
INLINE void outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %1,%0"::"dN"(port), "a"(value));
}

//写一个字
INLINE void outw(uint16_t port, uint16_t value) {
    asm volatile ("outw %1,%0"::"dN"(port), "a"(value));
}

// 写一个双字
INLINE void outl(uint16_t port, uint32_t value) {
    asm volatile ("outl %1,%0"::"dN"(port), "a"(value));
}


INLINE void io_wait(void) {
    // 等待一个 io 读写的时间
    asm volatile ( "outb %%al, $0x80" : : "a"(0));
}

// 设置 if 位
INLINE void enable_interrupt() {
    asm volatile ("sti");
}

INLINE void disable_interrupt() {
    asm volatile ("cli");
}

INLINE void halt() {
    asm volatile ("hlt");
}

INLINE void pause() {
    //停止固定的指令周期
    asm volatile ("pause");
}

INLINE uint32_t get_eflags() {
    uint32_t eflags;
    asm volatile (
    "pushf\n\t"
    "popl %0"
    :"=r"(eflags)
    ::"memory"
    );
    return eflags;
}

INLINE void set_eflags(uint32_t eflags) {
    asm volatile (
    "pushl %0\n\t"
    "popf"
    ::"rm"(eflags)
    :"memory"
    );
}

INLINE void enable_paging() {
    asm volatile (
    "mov %%cr0, %%eax       \n\t"
    "or  $0x80000000, %%eax \n\t"
    "mov %%eax, %%cr0"
    :: :"%eax");
}


INLINE bool is_paging() {
    uint32_t cr0;
    asm volatile("mov %%cr0, %0":"=a"(cr0));
    return (cr0 >> 31) == 1;
}

//获取产生错误的虚拟地址
INLINE pointer_t pf_addr() {
    uint32_t cr2;
    asm volatile("mov %%cr2, %0":"=a"(cr2));
    return cr2;
}


INLINE uint32_t cupid_available() {
#define CPUID_MASK (0b1 << 21)
    uint32_t eflags = get_eflags() | CPUID_MASK;
    set_eflags(eflags);
    return get_eflags() & CPUID_MASK;
}

INLINE uint32_t cpu_core() {
    uint32_t reg;
    asm volatile("cpuid":"=a"(reg):"a"(4));
    return ((reg >> 26) & 0x3f) + 1;
}

//刷新 tlb 缓存
INLINE void tlb_flush(pointer_t va) {
    __asm__ volatile ("invlpg (%0)" : : "a" (va));
}


#define INTERRUPT_MASK (0b1 << 9)

// gcc 优化屏障
#define opt_barrier() asm volatile("": : :"memory")

#endif //QUARKOS_X86_H
