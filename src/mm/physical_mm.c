//
// Created by pjs on 2021/2/1.
//
// 使用栈结构管理物理内存
#include "physical_mm.h"
#include "types.h"
#include "qlib.h"
#include "types.h"
#include "mm.h"
#include "multiboot2.h"
#include "qmath.h"

static struct stack {
    pointer_t *page;
    uint32_t top;
    uint32_t size; //page 数组末尾索引+1
} mm_page = {
        .page = NULL,
        .top = 0,
        .size = 0
};


static inline void push(pointer_t addr) {
    assertk(mm_page.top != mm_page.size);
    mm_page.page[mm_page.top++] = addr;
}

static inline pointer_t pop() {
    assertk(mm_page.top != 0);
    return mm_page.page[--mm_page.top];
}

//空闲页入栈
static void push_free_page() {
    multiboot_mmap_entry_t *entry;
    pointer_t tail = (pointer_t) g_mmap + g_mmap->size;

    // 空闲内存入栈
    for (entry = g_mmap->entries; (pointer_t) entry < tail; entry++) {
        assertk(entry->zero == 0);
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
            uint64_t length = entry->len;
            uint64_t start = entry->addr;
            //遇到小于 4k 的内存块直接舍弃
            while (length >= PAGE_SIZE) {
                push(start);
                start += PAGE_SIZE;
                length -= PAGE_SIZE;
            }
        }
    }

}

//初始 mm_page 内的栈
static void page_stack_init() {
    multiboot_mmap_entry_t *entry;
    pointer_t tail = (pointer_t) g_mmap + g_mmap->size;
    uint32_t stack_total = divUc(g_mem_total, PAGE_SIZE) * sizeof(pointer_t);
    for (entry = g_mmap->entries; (pointer_t) entry < tail; entry++)
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE && entry->len > stack_total) {
            uint64_t addr = entry->addr;

            entry->addr = SIZE_ALIGN(addr + stack_total);
            entry->len -= (entry->addr - addr);

            mm_page.page = (pointer_t *) addr;
            mm_page.size = (entry->addr - addr) / sizeof(pointer_t);
            break;
        }
    assertk(mm_page.page != NULL);
}

void phymm_init() {
    page_stack_init();
    push_free_page();
}


pointer_t phymm_alloc() {
    return pop();
}

void phymm_free(pointer_t addr) {
    push(PAGE_ADDR(addr));
}