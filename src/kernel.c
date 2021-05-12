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

#ifdef TEST

void *test_worker();

void create_test_worker();

#endif // TEST

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
    pit_init(PIT_TIMER_FREQUENCY);
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

    //    需要在多线程初始化之后
    //    ext2_init();

#ifdef TEST
    // kernel_main 所在线程成为 init 线程,一旦有其他线程可运行,
    // init 不会被调用,因此需要创建新的线程用于测试
    create_test_worker();
#endif // TEST

    enable_interrupt();

    idle();
}

#ifdef TEST

void *test_worker() {
    test_ide_rw();
//    test_dma_rw();
    test_thread();
    return NULL;
}

void create_test_worker() {
    kthread_t tid;
    kthread_create(&tid, test_worker, NULL);
}

#endif
