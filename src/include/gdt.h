//
// Created by pjs on 2021/1/20.
//

#ifndef QUARKOS_GDT_H
#define QUARKOS_GDT_H

#include "qstdint.h"
#include "qlib.h"

typedef struct gdt {
    uint16_t limit; // 段界限 0-15位, 置 0xffff
    uint16_t base1; // 段基地址 0-15 位, 置 0
    uint8_t base2;  // 段基地址16-23 位, 置 0
    uint8_t access; // access 位
    uint8_t mixed;  // 前4位为 limit 16-19 位,置 0xf ,后 4 位为标志位,置 0xc, 即mixed 置 0xcf
    uint8_t base3;  // 段基址 24-31 位, 置 0
} gdt_t;

static inline void set_gdt_base(gdt_t *gdt, uint32_t base) {
    static uint32_t base1 = generate_mask(11);
}

// 使用平坦模型
static inline void create_gdt(gdt_t *gdt) {
    //设置所有 base 为 0
    gdt->base1 = 0;
    gdt->base2 = 0;
    gdt->base3 = 0;

    // 设置 access 位

    // 4GB 虚拟内存
    gdt->limit = 0xffff;
    // 设置界限粒度为页,操作数大小为 32 位
    gdt->mixed = 0xcf;
}

#endif //QUARKOS_GDT_H
