//
// Created by pjs on 2021/2/1.
//
// 使用栈结构管理物理内存
#include "mm/physical_mm.h"
#include "types.h"
#include "klib/qlib.h"
#include "mm/mm.h"
#include "multiboot2.h"
#include "klib/qmath.h"

static struct mm_stack {
    pointer_t *page;
    uint32_t top;
    uint32_t size; //page 大小+1
} mm_page = {
        .page = NULL,
        .top = 0,
        .size = 0
};

__attribute__((always_inline))
static inline void push(pointer_t addr) {
    assertk(mm_page.top != mm_page.size);
    mm_page.page[mm_page.top++] = addr;
}

__attribute__((always_inline))
static inline pointer_t pop() {
    if (mm_page.top == 0) return PAGE_NULL;
    return mm_page.page[--mm_page.top];
}

//空闲页入栈
static void push_free_page() {
    // 空闲内存入栈
    for (multiboot_mmap_entry_t *entry = g_mmap->entries; (pointer_t) entry < g_mmap_tail; entry++) {
        assertk(entry->zero == 0);
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
            uint64_t length = entry->len;
            uint64_t start = SIZE_ALIGN(entry->addr);
            //遇到小于 4k 的内存块直接舍弃
            while (length >= PAGE_SIZE) {
                push(start);
                start += PAGE_SIZE;
                length -= PAGE_SIZE;
            }
        }
    }
}

//初始化 mm_page,为 mm_page.page 分配内存
static void page_stack_init() {
    mm_page.size = DIV_CEIL(g_mem_total, PAGE_SIZE);
    mm_page.page = (pointer_t *) split_mmap(mm_page.size * sizeof(pointer_t));
}

void phymm_init() {
    page_stack_init();
    push_free_page();
    test_physical_mm();
}


pointer_t phymm_alloc() {
    return pop();
}

void phymm_free(pointer_t addr) {
    push(PAGE_ADDR(addr));
}


void test_physical_mm() {
    pointer_t addr[3];
    size_t size = mm_page.top;
    addr[0] = phymm_alloc();
    assertk(mm_page.top == size - 1);
    phymm_free(addr[0]);
    assertk(mm_page.top == size);

    addr[1] = phymm_alloc();
    assertk(addr[1] == addr[0]);
    test_pass;
}