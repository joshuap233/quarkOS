#include <stddef.h> // size_t and NULL
#include "qstdint.h" // ntx_t and uintx_t
#include "qstring.h"
#include "qlib.h"
#include "multiboot2.h"
#include "qmath.h"
#include "gdt.h"
#include "idt.h"
#include "vga.h"

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

void kernel_main(void) {
    gdt_init();
    idt_init();
    vga_init();
    printfk("hello word\n");
//    uint32_t t= 1/0;
//    printfk("%x\n", (uint32_t) _startKernel);
//    parse_multiboot_info_struct();
}