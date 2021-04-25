//
// Created by pjs on 2021/1/23.
//
#include "gdt.h"
#include "lib/qstring.h"

uint32_t gdt_index = 1; //第一个空闲gdt索引
static gdt_entry_t _Alignas(8) gdt[GDT_COUNT] = {0};

//用于设置全局描述符
void gdt_set_(gdt_entry_t *_gdt, uint32_t base, uint32_t limit, uint8_t flag, uint8_t access) {
    // 设置基地址
    _gdt->base_l = base & (MASK_U32(16));
    _gdt->base_m = (base >> 16) & (MASK_U32(8));
    _gdt->base_h = base >> 24;

    // 设置 limit
    _gdt->limit = limit & (MASK_U32(16));
    _gdt->limit_h = limit >> 16;
    // 设置 G,D/B,L,AVL 四个 FLAG
    _gdt->flag = flag;

    // 设置访问位
    _gdt->access = access;
}

void gdt_init() {
    // 使用平坦模型,
    gdtr_t gdtr = {
            .limit = GDT_COUNT * GDT_SIZE - 1,
            .address = (ptr_t) gdt
    };
    // 第 0 个 gdt 为 NULL(0)
    gdt_set_(&gdt[1], 0, GDT_LIMIT, GDT_FLAG, GDT_SYS_CODE); //内核代码段
    gdt_set_(&gdt[2], 0, GDT_LIMIT, GDT_FLAG, GDT_SYS_DATA); //内核数据段
    gdt_set_(&gdt[3], 0, GDT_LIMIT, GDT_FLAG, GDT_USR_CODE); //用户代码段
    gdt_set_(&gdt[4], 0, GDT_LIMIT, GDT_FLAG, GDT_USR_DATA); //用户数据段
    gdt_index = 5;

    assertk(gdt[1].limit == 0xffff)
    assertk(gdt[1].base_l == 0)
    assertk(gdt[1].base_m == 0)
    assertk(gdt[1].base_h == 0)
    assertk(gdt[1].limit_h == 0xf)
    assertk(gdt[1].flag == 0xc)
    assertk(gdt[1].access == GDT_SYS_CODE)
    // 调用函数,参数压栈,返回地址压栈,因此参数地址 = (esp+4)
    gdtr_set((ptr_t) &gdtr);
}

//可以用于设置 tss ldt 等
void gdt_set(uint32_t index, ptr_t value_addr) {
    q_memcpy(&gdt[index], (void *) value_addr, sizeof(uint64_t));
}