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
} __attribute__((packed)) gdt_t;
//一定要加上  __attribute__((packed)), 否则这些值不会紧挨在一起

typedef struct gdtr {
    uint16_t limit;
    uint32_t address;
} __attribute__((packed)) gdtr_t;

void gdt_set(gdt_t *gdt, uint32_t base, uint32_t limit, uint8_t flag, uint8_t access);

void set_gdtr(uint32_t gdtr);

void gdt_init();

#endif //QUARKOS_GDT_H