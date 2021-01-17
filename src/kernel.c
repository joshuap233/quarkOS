#include <stddef.h> // size_t and NULL
#include "qstdint.h" // ntx_t and uintx_t
#include "qstring.h"
#include "qlib.h"

#if defined(__linux__)
#warning "你没有使用跨平台编译器进行编译"
#endif

#if !defined(__i386__)
#warning "你没有使用 ix86-elf 编译器进行编译"
#endif

#if !defined(__STDC_HOSTED__)
#error "你没有使用 freestanding 模式"
#endif


void get_multiboot_info_struct() {
    register uint32_t ebx asm("ebx");
    // Boot information address
    uint32_t *bia = (uint32_t *) ebx;

    printfk("P: %p\n", bia);

    uint32_t *max_addr = bia + *bia / sizeof(uint32_t);

    // reserved 字段恒为 0
    assertk(*(bia + 1) == 0);

    // 标签首地址
    uint32_t *tag = bia + 2;
    // bia 总长度
    uint32_t tag_total, tag_type = *tag;


    while (tag_type != 6 && tag < max_addr) {
        printfk("type: %u\n", tag_type);

        tag_total = *(tag + 1);
        //Boot information 的 tags以 8 字节对齐
        tag_total = tag_total + (8 - tag_total % 8);
        // 下一个标签地址
        tag = tag + tag_total / sizeof(uint32_t);
        tag_type = *tag;
    }
}


void kernel_main(void) {
    terminal_initialize();
//    print_str("Hello, kernel World!\n");
    get_multiboot_info_struct();
}