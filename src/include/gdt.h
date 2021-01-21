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
    uint8_t lim_flag;  // 前4位为 limit 16-19 位,置 0xf ,后 4 位为标志位,置 0xc, 即mixed 置 0xcf
    uint8_t base3;  // 段基址 24-31 位, 置 0
#define GDT_LIMIT 0xfffff
#define GDT_FLAG 0b1100     // 设置 32 位处理器,操作数为 32 位,界限粒度为页
} gdt_t;

static inline void gdt_set_base(gdt_t *gdt, uint32_t base) {
    gdt->base1 = base & (MASK_U32(16));
    gdt->base2 = (base >> 16) & (MASK_U32(8));
    gdt->base3 = base >> 24;
}

static inline void gdt_set_limit(gdt_t *gdt, uint32_t limit) {
    gdt->limit = limit & (MASK_U32(16));
    gdt->lim_flag = gdt->lim_flag | (limit >> 16);
}

static inline void gdt_set_flag(gdt_t *gdt, uint8_t flag) {
    // 设置 G,D/B,L,AVL 四个 FLAG
    gdt->lim_flag = gdt->lim_flag | (flag << 4);
}

// 使用平坦模型,header 为 GDT 首地址
static inline void gdt_create(void *header) {
    gdt_t **gdt = (gdt_t **) header;
    q_bzero(gdt, 5 * sizeof(gdt_t));
    gdt_set_base(gdt[0], 0);
    gdt_set_limit(gdt[0], GDT_LIMIT);
    gdt_set_flag(gdt[0], GDT_FLAG);

    printfk("%x\n", gdt[0]->limit);
    printfk("%x\n", gdt[0]->lim_flag);

    assertk(gdt[0]->limit == 0xffff);
    assertk(gdt[0]->base1 == 0);
    assertk(gdt[0]->base2 == 0);
    assertk(gdt[0]->base3 == 0);
    assertk(gdt[0]->lim_flag == 0xcf);
}

#endif //QUARKOS_GDT_H
