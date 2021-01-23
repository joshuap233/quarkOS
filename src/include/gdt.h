//
// Created by pjs on 2021/1/20.
//

#ifndef QUARKOS_GDT_H
#define QUARKOS_GDT_H

#include "qstdint.h"
#include "qlib.h"

typedef struct gdt {
    uint16_t limit; // 段界限 0-15位, 置 0xffff
    uint16_t base_l; // 段基地址 0-15 位, 置 0
    uint8_t base_m;  // 段基地址16-23 位, 置 0
    uint8_t access; // access 位
    uint8_t limit_h: 4;  // limit 16-19 位,置 0xf ,
    uint8_t flag: 4;  // 标志位,置 0xc
    uint8_t base_h;  // 段基址 24-31 位, 置 0
#define GDT_SIZE         sizeof(struct gdt)
#define GDT_LIMIT        0xfffff
#define GDT_FLAG         0b1100  // 设置 32 位处理器,操作数为 32 位,界限粒度为页
#define GDT_SYS_CODE     0x9a    // 系统代码段
#define GDT_SYS_DATA     0x92    // 系统数据段
#define GDT_SYS_RODATA   0x90    // 系统只读数据段
#define GDT_USR_CODE     0xfa    // 用户代码段
#define GDT_USR_DATA     0xf2    // 用户数据段
#define GDT_USR_RODATA   0xf0    // 用户只读数据段
} gdt_t;
#define GDT_COUNT 7

typedef struct gdtr {
    uint16_t size;
    uint32_t address;
} gdtr_t;

static void gdt_set(gdt_t *gdt, uint32_t base, uint32_t limit, uint8_t flag, uint8_t access) {
    // 设置基地址
    gdt->base_l = base & (MASK_U32(16));
    gdt->base_m = (base >> 16) & (MASK_U32(8));
    gdt->base_h = base >> 24;

    // 设置 limit
    gdt->limit = limit & (MASK_U32(16));
    gdt->limit_h = limit >> 16;
    // 设置 G,D/B,L,AVL 四个 FLAG
    gdt->flag = flag;

    // 设置访问位
    gdt->access = access;
}

static inline void gdt_init() {
    // 使用平坦模型,
    // 第 0 个 gdt 为 NULL,
    // 1-3 gdt 为内核段: text,data,rodata
    // 4-6 gdt 为用户段: text,data,rodata
    static gdt_t gdt[GDT_COUNT] = {0};

    // 设置 gdtr
    gdtr_t gdtr;
    gdtr.size = GDT_COUNT * GDT_SIZE;
    gdtr.address = (uint32_t) gdt;

    //    printfk("gdtr: %lx", gdtr);
    gdt_set(&gdt[1], 0, GDT_LIMIT, GDT_FLAG, GDT_SYS_CODE);
    gdt_set(&gdt[2], 0, GDT_LIMIT, GDT_FLAG, GDT_SYS_DATA);
    gdt_set(&gdt[3], 0, GDT_LIMIT, GDT_FLAG, GDT_SYS_RODATA);
    gdt_set(&gdt[4], 0, GDT_LIMIT, GDT_FLAG, GDT_USR_CODE);
    gdt_set(&gdt[5], 0, GDT_LIMIT, GDT_FLAG, GDT_USR_DATA);
    gdt_set(&gdt[6], 0, GDT_LIMIT, GDT_FLAG, GDT_USR_RODATA);


    assertk(gdt[1].limit == 0xffff)
    assertk(gdt[1].base_l == 0)
    assertk(gdt[1].base_m == 0)
    assertk(gdt[1].base_h == 0)
    assertk(gdt[1].limit_h == 0xf)
    assertk(gdt[1].flag == 0xc)
    assertk(gdt[1].access == GDT_SYS_CODE)

    asm (
    "lgdt (%0) \n\t"
    "mov %1,%%ds \n\t"
    "mov %1,%%es \n\t"
    "mov %1,%%fs \n\t"
    "mov %1,%%gs \n\t"
    "mov %1,%%ss \n\t"
    "jmp $0x08,$foo \n\t"  // 设置 cs 寄存器并清空流水线
    "foo: "
    ::"r"((uint32_t) &gdtr), "a"(0x10)
    );
}

#endif //QUARKOS_GDT_H
