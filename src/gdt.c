//
// Created by pjs on 2021/1/23.
//
#include "gdt.h"

#define GDT_COUNT 7

void gdt_init() {
    // 使用平坦模型,
    // 第 0 个 gdt 为 NULL,
    // 1-3 gdt 为内核段: text,data,rodata
    // 4-6 gdt 为用户段: text,data,rodata
    static gdt_t gdt[GDT_COUNT] = {0};

    // 设置 gdtr
    gdtr_t gdtr = {
            .limit = GDT_COUNT * GDT_SIZE - 1,
            .address = (uint32_t) gdt
    };

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
    // 调用函数,参数压栈,返回地址压栈,因此参数地址 = (esp+4)
    set_gdtr((uint32_t) &gdtr);
}

void gdt_set(gdt_t *gdt, uint32_t base, uint32_t limit, uint8_t flag, uint8_t access) {
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
