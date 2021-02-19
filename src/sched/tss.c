//
// Created by pjs on 2021/2/14.
//
#include "tss.h"

#include "gdt.h"

static tss_t tss = {0};
static tss_desc_t tss_desc = {0};

void tss_set_desc() {
    pointer_t base = (pointer_t) &tss;
    pointer_t limit = sizeof(tss);

    tss_desc.base_l = base & (MASK_U32(16));
    tss_desc.base_m = (base >> 16) & (MASK_U32(8));
    tss_desc.base_h = base >> 24;

    tss_desc.limit = limit & (MASK_U32(16));
    tss_desc.limit_h = limit >> 16;

    // 设置 G,D/B,L,AVL 四个 FLAG
    tss_desc.flag = TSS_FLAG;

    // 设置访问位
    tss_desc.access = TSS_NBUSY | TSS_USER | TSS_PRES;
}


//初始化内核任务
void tss_init() {
    register uint32_t esp0 asm("esp");
    uint32_t tss_desc_index = get_free_gdt_index();
    tss.esp0 = esp0;
    tss.ss0 = 0x10;//内核数据段选择子
    tss_set_desc();

    gdt_set(tss_desc_index, (pointer_t) &tss_desc);
    tr_set((tss_desc_index << 3) & 0b011);
}

