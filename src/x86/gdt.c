//
// Created by pjs on 2021/1/23.
//
#include <gdt.h>
#include <lib/qstring.h>


static gdt_entry_t _Alignas(8) gdt[GDT_COUNT] = {0};

extern void tr_set(uint32_t tss_desc_index);

extern void gdtr_set(uint32_t gdtr);

static void tss_desc_set();

//用于设置全局描述符
static void gdt_set(u32_t index, uint32_t base, uint32_t limit, uint8_t flag, uint8_t access) {
    gdt_entry_t *entry = &gdt[index];

    // 设置基地址
    entry->base_l = base & (MASK_U32(16));
    entry->base_m = (base >> 16) & (MASK_U32(8));
    entry->base_h = base >> 24;

    // 设置 limit
    entry->limit = limit & (MASK_U32(16));
    entry->limit_h = limit >> 16;

    // 设置 G,D/B,L,AVL 四个 FLAG
    entry->flag = flag;

    // 设置访问位
    entry->access = access;
}

void gdt_init() {
    // 使用平坦模型,
    gdtr_t gdtr = {
            .limit = GDT_COUNT * GDT_SIZE - 1,
            .address = (ptr_t) gdt
    };
    // 第 0 个 gdt 为 NULL(0)
//    gdt_set(SEL_NULL, 0, 0, 0, 0); //内核代码段
    gdt_set(SEL_KTEXT, 0, GDT_LIMIT, GDT_FLAG, GDT_SYS_CODE); //内核代码段
    gdt_set(SEL_KDATE, 0, GDT_LIMIT, GDT_FLAG, GDT_SYS_DATA); //内核数据段
    gdt_set(SEL_UTEXT, 0, GDT_LIMIT, GDT_FLAG, GDT_USR_CODE); //用户代码段
    gdt_set(SEL_UDATA, 0, GDT_LIMIT, GDT_FLAG, GDT_USR_DATA); //用户数据段

    assertk(gdt[1].limit == 0xffff)
    assertk(gdt[1].base_l == 0)
    assertk(gdt[1].base_m == 0)
    assertk(gdt[1].base_h == 0)
    assertk(gdt[1].limit_h == 0xf)
    assertk(gdt[1].flag == 0xc)
    assertk(gdt[1].access == GDT_SYS_CODE)

    tss_desc_set();
    gdtr_set((ptr_t) &gdtr);
    tr_set(SEL_TSS << 3);
}


static tss_t tss = {0};

static void tss_desc_set() {
    ptr_t base = (ptr_t) &tss;
    ptr_t limit = sizeof(tss_t);

    gdt_set(SEL_TSS, base, limit, TSS_FLAG, TSS_NBUSY | TSS_KERNEL | TSS_PRES);

    //TODO:
    register uint32_t esp0 asm("esp");
    tss.esp0 = esp0;
    tss.ss0 = 0x10;         // 内核数据段选择子

    tss.cs = SEL_UTEXT << 3;
    tss.ss = SEL_UDATA << 3;
    tss.ds = SEL_UDATA << 3;
    tss.es = SEL_UDATA << 3;
    tss.fs = SEL_UDATA << 3;
    tss.gs = SEL_UDATA << 3;
}
