#include <types.h>
#include <multiboot2.h>
#include <mm/init.h>
#include <sched/kthread.h>
#include <drivers/init.h>
#include <isr.h>
#include <sched/init.h>
#include <lib/qlib.h>
#include <fs/init.h>
#include <mm/mm.h>
#include <mm/kvm.h>

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
    pmm_init();
    slab_init();
    kvm_init();

    ide_init();
//    dma_init();

    scheduler_init();
    thread_init();
    cmos_init();

    page_cache_init();

    enable_interrupt();

    // 需要在中断开启之后
    vfs_init();
    ext2_init();

#ifdef TEST
//    test_ide_rw();
//    test_dma_rw();
//    test_vfs();

    test_thread();
#endif // TEST

    block_thread(NULL, NULL);
}

/** 映射临时页表 **/
static _Alignas(KTHREAD_STACK_SIZE) u8_t kStack[KTHREAD_STACK_SIZE];

static uint32_t kStack_top = (ptr_t) kStack + KTHREAD_STACK_SIZE;

extern pdr_t *boot_page_dir;
extern ptb_t *boot_page_table1;
extern ptb_t *boot_page_table2;

// 映射临时页表
__attribute__((section(".init.text"))) void map_tmp_page(void) {
    // 映射物理地址 0-4M 到虚拟地址 0 - 4M 与 HIGH_MEM ~ HIGH_MEM + 4M
    // 否则开启分页后,无法运行 init 代码
    boot_page_dir->entry[0] = (ptr_t) boot_page_table1 | VM_PRES | VM_KW;
    boot_page_dir->entry[1023] = (ptr_t) boot_page_table2 | VM_PRES | VM_KW;

    for (int i = 0; i < 1024; ++i) {
        boot_page_table1->entry[i] = (i << 12) | VM_PRES | VM_KW;
    }

    for (int i = 0; i < 1024; ++i) {
        boot_page_table2->entry[i] = (i << 12) | VM_PRES | VM_KW;
    }

    lcr3((ptr_t) boot_page_dir);
}


__attribute__((section(".init.text"))) void kern_entry(void) {
    map_tmp_page();

    enable_paging();

    // 切换到新栈, ebp 清 0 用于追踪栈底
    __asm__ volatile ("mov %0, %%esp\n\t"
                      "xor %%ebp, %%ebp" : : "r" (kStack_top));

    // 更新全局 mba 地址
    mba = (multiboot_info_t *) ((ptr_t) mba + KERNEL_START);

    kernel_main();
}

