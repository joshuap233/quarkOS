//
// Created by pjs on 2021/1/20.
//

#ifndef QUARKOS_GDT_H
#define QUARKOS_GDT_H

#include <types.h>
#include <lib/qlib.h>

#define GDT_SIZE         sizeof(struct gdt_entry)
#define GDT_LIMIT        0xfffff
#define GDT_FLAG         0b1100  // 设置 32 位处理器,操作数为 32 位,界限粒度为页
#define GDT_SYS_CODE     0x9a    // 系统代码段
#define GDT_SYS_DATA     0x92    // 系统数据段
#define GDT_SYS_RODATA   0x90    // 系统只读数据段
#define GDT_USR_CODE     0xfa    // 用户代码段
#define GDT_USR_DATA     0xf2    // 用户数据段
#define GDT_USR_RODATA   0xf0    // 用户只读数据段
#define GDT_COUNT 20


#define TSS_BUSY   0b1011
#define TSS_NBUSY  0b1001
#define TSS_PRES   (1 << 7)
#define TSS_KERNEL (0b00 << 5)  //特权级
#define TSS_USER   (0b11 << 5)  //特权级
#define TSS_FLAG   0            //不分页

typedef struct gdt_entry {
    uint16_t limit;      // 段界限 0-15位, 置 0xffff
    uint16_t base_l;     // 段基地址 0-15 位, 置 0
    uint8_t base_m;      // 段基地址16-23 位, 置 0
    uint8_t access;      // access 位
    uint8_t limit_h: 4;  // limit 16-19 位,置 0xf ,
    uint8_t flag: 4;     // 标志位,置 0xc
    uint8_t base_h;      // 段基址 24-31 位, 置 0
} PACKED gdt_entry_t;

typedef struct gdtr {
    uint16_t limit;
    uint32_t address;
} PACKED gdtr_t;



// 不使用 x86 提供的任务切换机制,
// 只使用 esp0 与 ss0
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
    uint16_t : 16;

    uint32_t T: 1;      //如果设置该位,切换到当前任务时触发debug异常
    uint32_t : 15;
    uint32_t io_map_ba: 16;
}PACKED tss_t;

// 段描述符编号
#define SEL_NULL   0
#define SEL_KTEXT  1
#define SEL_KDATE  2
#define SEL_UTEXT  3
#define SEL_UDATA  4
#define SEL_TSS    5

#endif //QUARKOS_GDT_H
