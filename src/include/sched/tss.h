//
// Created by pjs on 2021/2/14.
//

#ifndef QUARKOS_TSS_H
#define QUARKOS_TSS_H

#include "types.h"

//tss 描述符结构
typedef struct tss_desc {
    uint16_t limit; // 段界限 0-15位, 置 0xffff
    uint16_t base_l; // 段基地址 0-15 位, 置 0
    uint8_t base_m;  // 段基地址16-23 位, 置 0
    uint8_t access;
    uint8_t limit_h: 4;  // limit 16-19 位,置 0xf ,
    uint8_t flag: 4;  // 标志位,置 0xc
    uint8_t base_h;  // 段基址 24-31 位, 置 0
#define TSS_BUSY   0b1011 //A busy task is currently running or suspended.
#define TSS_NBUSY  0b1001
#define TSS_PRES   1 << 7
#define TSS_NPRES  0 << 7
#define TSS_KERNEL 0b00 << 5  //特权级
#define TSS_USER   0b11 << 5  //特权级
#define TSS_FLAG   0          //不分页
} PACKED tss_desc_t;

// tss 段应该为标记为可读写
//tss 段结构
// 不使用 x86 提供的任务切换机制,那么只需要使用 prev,esp0与 ss0
typedef struct tss {
    uint32_t prev: 16;
    uint16_t : 16;

    uint32_t esp0;

    uint32_t ss0: 16;
    uint16_t : 16;

    uint32_t esp1;

    uint32_t ss1: 16;
    uint16_t : 16;

    uint32_t esp2;

    uint32_t ss2: 16;
    uint16_t : 16;

    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;

    uint32_t es: 16;
    uint32_t : 16;

    uint32_t cs: 16;
    uint32_t : 16;

    uint32_t ss: 16;
    uint32_t : 16;

    uint32_t ds: 16;
    uint32_t : 16;

    uint32_t fs: 16;
    uint32_t : 16;

    uint32_t gs: 16;
    uint32_t : 16;

    uint32_t ldt_ss: 16;
    //    Contains the segment selector for the task's LDT.
    uint16_t : 16;

    uint32_t T: 1; //如果设置该位,切换到当前任务时触发debug异常
    uint32_t : 15;
    uint32_t io_map_ba: 16;
    //Contains a 16-bit offset from the base of the TSS to the I/O permission bit
    //map and interrupt redirection bitmap
}PACKED tss_t;

extern void tr_set(uint32_t tss_desc_index);
#endif //QUARKOS_TSS_H
