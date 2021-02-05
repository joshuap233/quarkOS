//
// Created by pjs on 2021/2/1.
//
// 使用栈结构管理物理内存
#include "physical_mm.h"
#include "types.h"
#include "qlib.h"
#include "types.h"
#include "mm.h"


static struct stack {
    pointer_t page[N_PPAGE];
    uint32_t top;
    uint32_t size;
} mm_page = {
        .top=0,
        .size = N_PPAGE
};


static inline void push(pointer_t addr) {
    assertk(mm_page.top != mm_page.size);
    mm_page.page[mm_page.top++] = addr;
}

static inline pointer_t pop() {
    assertk(mm_page.top != 0);
    return mm_page.page[--mm_page.top];
}

//start,length 分别为一块内存的开始地址与长度
void phymm_init(pointer_t start, pointer_t length) {
    //不管理内核代码区域与低于 1M 的内存(内核被加载到 1M 处)
    if (start < K_END) {
        if (start + length < K_SIZE) return;
        start = K_END;
        length -= K_SIZE;
    }

    start = ADDR_ALIGN(start);

    //TODO: 遇到小于 4k 的内存块直接舍弃?
    while (length >= PAGE_SIZE) {
        push(start);
        start += PAGE_SIZE;
        length -= PAGE_SIZE;
    }
}

pointer_t phymm_alloc() {
    return pop();
}

void phymm_free(pointer_t addr) {
    push(PAGE_ADDR(addr));
}