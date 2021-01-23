#include <stddef.h> // size_t and NULL
#include "qstdint.h" // ntx_t and uintx_t
#include "qstring.h"
#include "qlib.h"
#include "multiboot2.h"
#include "qmath.h"
#include "gdt.h"

#if defined(__linux__)
#warning "你没有使用跨平台编译器进行编译"
#endif

#if !defined(__i386__)
#warning "你没有使用 x86-elf 编译器进行编译"
#endif

#if !defined(__STDC_HOSTED__)
#warning "你没有使用 freestanding 模式"
#endif

// 内核最大地址+1, 不要修改成 *_end,
extern char _endKernel[], _startKernel[];
typedef struct multiboot_tag_mmap multiboot_tag_mmap_t;
typedef struct multiboot_tag_apm multiboot_tag_apm_t;
typedef struct multiboot_tag multiboot_tag_t;
typedef struct multiboot_mmap_entry multiboot_mmap_entry_t;

void parse_memory_map(multiboot_tag_mmap_t *mmap) {
    multiboot_mmap_entry_t *entry = mmap->entries;
    char *tail = (char *) mmap + mmap->size;
    while ((char *) entry < tail) {
        assertk(entry->zero == 0);
        switch (entry->type) {
            case MULTIBOOT_MEMORY_AVAILABLE:
                printfk("base_addr: %x\n", (uint32_t) entry->addr);
                printfk("length   : %x\n", (uint32_t) entry->len);
                break;
        }
        entry = entry + 1;
    }
}

void parse_multiboot_info_struct() {
    register uint32_t ebx asm("ebx");
    // bia 为 Boot information 头
    uint32_t *bia;
    multiboot_tag_t *tag;
    multiboot_tag_mmap_t *mmap;
    multiboot_tag_apm_t *apm;
    uint32_t flag = 0;

    bia = (uint32_t *) ebx;
    //指向最大地址,*bia 为信息结构总大小
    char *tail = (char *) bia + *bia;
    // 保留字段恒为 0
    assertk(*(bia + 1) == 0);
    // 第一个标签首地址
    tag = (multiboot_tag_t *) (bia + 2);

    while ((char *) tag < tail) {
        switch (tag->type) {
            case MULTIBOOT_TAG_TYPE_MMAP:
                mmap = (multiboot_tag_mmap_t *) tag;
                flag++;
                break;
            case MULTIBOOT_TAG_TYPE_APM:
                apm = (multiboot_tag_apm_t *) tag;
                flag++;
                break;
        }
        if (flag == 2) break;
        //Boot information 的 tags以 8 字节对齐
        // multiboot_tag 大小为 8 字节
        tag = tag + divUc(tag->size, 8);
    }

    assertk(mmap->type == MULTIBOOT_TAG_TYPE_MMAP);
    assertk(mmap->entry_version == 0);
    assertk(apm->type == MULTIBOOT_TAG_TYPE_APM);
    parse_memory_map(mmap);
}


void kernel_main(void) {
    gdt_init();
    terminal_initialize();

//    printfk("%x\n", (uint32_t) _startKernel);
//    parse_multiboot_info_struct();
}