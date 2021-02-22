#include <stddef.h> // size_t and NULL
#include "types.h" // ntx_t and uintx_t
#include "qstring.h"
#include "qlib.h"
#include "multiboot2.h"
#include "qmath.h"
#include "gdt.h"
#include "idt.h"
#include "vga.h"
#include "x86.h"
#include "keyboard.h"
#include "mm.h"
#include "kthread.h"

#if defined(__linux__)
#warning "你没有使用跨平台编译器进行编译"
#endif

#if !defined(__i386__)
#warning "你没有使用 x86-elf 编译器进行编译"
#endif

#if !defined(__STDC_HOSTED__)
#warning "你没有使用 freestanding 模式"
#endif


void hello() {
    char space[] = "                ";
    printfk("\n");
    printfk("%s************************************************\n", space);
    printfk("%s*                                              *\n", space);
    printfk("%s*                                              *\n", space);
    printfk("%s*              Welcome to Quark OS             *\n", space);
    printfk("%s*                                              *\n", space);
    printfk("%s*                                              *\n", space);
    printfk("%s************************************************\n", space);
    printfk("\n");
}

void *workerA(void *args) {
    disable_interrupt();
    printfk("a\n");
    enable_interrupt();
    return NULL;
}
void *workerB(void *args) {
    disable_interrupt();
    printfk("b\n");
    enable_interrupt();
    return NULL;
}

//mba 为 multiboot info struct 首地址
void kernel_main(multiboot_info_t *mba, uint32_t magic) {
    vga_init();
    assertk(magic == 0x36d76289);
    assertk(mba->zero == 0);
    multiboot_init(mba);
    hello();
    gdt_init();
    idt_init();
    mm_init();
//    sched_init();
//    kthread_create(workerA, NULL);
//    kthread_create(workerA, NULL);
//    kthread_create(workerA, NULL);
//    kthread_create(workerB, NULL);
    while (1) {
        kb_getchar();
    }
}