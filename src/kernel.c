#include "types.h"
#include "multiboot2.h"
#include "mm/init.h"
#include "sched/kthread.h"
#include "drivers/init.h"
#include "isr.h"
#include "sched/init.h"
#include "lib/qlib.h"
#include "fs/init.h"


#ifdef __linux__
#error "你没有使用跨平台编译器进行编译"
#endif //__linux__

#ifndef __i386__
#error "你没有使用 x86-elf 编译器进行编译"
#endif //__i386__

#ifndef __STDC_HOSTED__
#error "你没有使用 ffreestanding 模式"
#endif //__STDC_HOSTED__


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


extern multiboot_info_t *mba;
extern uint32_t magic;

//mba 为 multiboot info struct 首地址
void kernel_main() {
    vga_init();
    assertk(magic == 0x36d76289);
    assertk(mba->zero == 0);
    multiboot_init(mba);
    memBlock_init();
    hello();
    gdt_init();

    // 中断初始化
    idt_init();
    pic_init(32, 40);
    pit_init();
    // ps2 设备初始化
    ps2_init();
    kb_init();

    // 内存管理模块初始化
    vm_area_init();
    pmm_init();
    slab_init();
    vmm_init();

    ide_init();
//    dma_init();

    scheduler_init();
    thread_init();
    cmos_init();

    page_cache_init();

    enable_interrupt();

    // 需要在中断开启之后
    ext2_init();
    vfs_init();

#ifdef TEST
//    test_ide_rw();
//    test_dma_rw();
    test_ext2();

    test_thread();
#endif // TEST

    block_thread(NULL, NULL);
}
