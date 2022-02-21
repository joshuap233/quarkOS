#include <types.h>
#include <multiboot2.h>
#include <mm/init.h>
#include <mm/mm.h>
#include <mm/vm.h>
#include <task/init.h>
#include <task/fork.h>
#include <drivers/init.h>
#include <lib/qlib.h>
#include <fs/init.h>
#include <isr.h>
#include <terminal.h>
#include <drivers/mp.h>
#include <gdt.h>
#include <mm/page_alloc.h>
#include <mm/kmalloc.h>

#ifdef __linux__
#error "你没有使用跨平台编译器进行编译"
#endif //__linux__

#ifndef __i386__
#error "你没有使用 x86-elf 编译器进行编译"
#endif //__i386__

#ifndef __STDC_HOSTED__
#error "你没有使用 ffreestanding 模式"
#endif //__STDC_HOSTED__

static multiboot_info_t *mba;
static uint32_t magic;
static void startAp();
extern void cpu_init();

// mba 为 multiboot info struct 首地址
void kernel_main() {
    vga_init();
    assertk(magic == 0x36d76289);
    assertk(mba->zero == 0);

    multiboot_init(mba);
    memBlock_init();

    gdt_init();
    kvm_init();

    acpi_init();
    smp_init();
    lapic_init();
    ioapic_init();

    pmm_init();
    slab_init();

    // 中断初始化
    idt_init();

    // ps2 设备初始化
    ps2_init();
    kb_init();
    terminal_init();

    // 磁盘驱动初始化
    ide_init();
    // dma_init();
    // nic_init();
    scheduler_init();
    task_init();

    page_cache_init();
    vfs_init();

    cmos_init();
    enable_interrupt();
    cpu_init();

    ext2_init();

#ifdef TEST
//    test_ide_rw();
//    test_dma_rw();
//    test_vfs();

//    test_thread();
#endif // TEST

    startAp();
    // 初始化用户任务后,当前的栈将被第一个用户任务用作内核栈,
    // 栈内容将被中断数据覆盖,user_task_init 后的函数可用
    user_task_init();
    task_sleep(NULL,NULL);
}

INLINE void switch_stack(){
    register u32_t esp asm("esp");
    void *stack = kmalloc(STACK_SIZE);
    memcpy(stack, (void*)PAGE_ADDR((ptr_t)esp),STACK_SIZE);
    // 切换栈
    asm volatile ("mov %0, %%esp": :"r" (stack+((ptr_t)esp & PAGE_MASK)));
}

void ap_main() {
    extern void load_gdt();
    extern void load_cr3();
    extern void task_init1();
    load_gdt();
    load_cr3();
    lapic_init();
    load_idtr();
    switch_stack();
    task_init1();
    getCpu()->start = true;
    task_set_time_slice(CUR_TCB->pid,0);
    getCpu()->idle = &CUR_TCB->run_list;
    enable_interrupt();
    idle();
}

// 放到 ap 段, 标记段为 loadable
// 没找到 linker 脚本标记 loadable 的方法.就这样吧..
SECTION(".ap.foo")
UNUSED static u8_t foo(){
    return 1;
}

/**** 映射临时页表  ****/

// 内核栈
static _Alignas(STACK_SIZE) SECTION(".init.data")
pde_t boot_page_dir[N_PDE] = {[0 ...N_PDE - 1]=VM_NPRES};

static _Alignas(PAGE_SIZE) SECTION(".init.data")
pte_t boot_page_table1[N_PTE] = {[0 ...N_PDE - 1]=VM_NPRES};

static _Alignas(PAGE_SIZE) SECTION(".init.data")
pte_t boot_page_table2[N_PTE] = {[0 ...N_PDE - 1]=VM_NPRES};


// 临时 mba 与 magic
SECTION(".init.data") multiboot_info_t *tmp_mba;
SECTION(".init.data") uint32_t tmp_magic;

// 映射临时页表
SECTION(".init.text")
void map_tmp_page(void) {
    // 映射物理地址 0-4M 到虚拟地址 0 - 4M 与 KERNEL_START ~ KERNEL_START + 4M
    // 否则开启分页后,无法运行 init 代码
    boot_page_dir[0] = (ptr_t) &boot_page_table1 | VM_PRES | VM_KRW;
    boot_page_dir[PDE_INDEX(KERNEL_START)] = (ptr_t) &boot_page_table2 | VM_PRES | VM_KRW;

    for (int i = 0; i < 1024; ++i) {
        boot_page_table1[i] = (i << 12) | VM_PRES | VM_KRW;
        boot_page_table2[i] = (i << 12) | VM_PRES | VM_KRW;
    }

    // 不能直接用 x86.h 的 lcr3 与 enable_paging
    asm volatile("movl %0,%%cr3" : : "r" (&boot_page_dir));
}

// 继续用 grub2 设置的 gdt
SECTION(".init.text")
void kernel_entry(void) {

    map_tmp_page();

    // 开启分页
    asm volatile (
    "mov %%cr0, %%eax       \n\t"
    "or  $0x80000000, %%eax \n\t"
    "mov %%eax, %%cr0"
    :: :"%eax");

    // stack + KERNEL_START
    asm volatile(
    "addl %%esp, %%eax \n\t"
    "movl %%eax, %%esp"
    ::"a"(KERNEL_START));

    // mba 在 data 节,而 tmp_mba 在 init 节,
    // data 节分页开启前无法直接访问,因此需要先设置 tmp_mba
    mba = (void *) tmp_mba + KERNEL_START;
    magic = tmp_magic;

    kernel_main();
}

// kStack2 需要同时在临时页表与内核页表中映射,因此使用静态分配
static _Alignas(STACK_SIZE)
u8_t kStack2[STACK_SIZE];

static void startAp() {
    extern void lapicStartAp(u8_t apicid, u32_t addr);
    extern char init_struct[];

    extern char ap_start[];

    u32_t *init = (void *)((ptr_t)init_struct + KERNEL_START);
    // 内核页表的 VM 在 3G 以上,需要用临时页表
    init[0]= (u32_t)boot_page_dir;

    for (int i = 0; i < cpuCfg.nCpu; ++i) {
        if (&cpus[i] == getCpu()) {
            continue;
        }
        init[1] = (ptr_t)&kStack2 + STACK_SIZE;
        lapicStartAp(cpus[i].apic_id, (ptr_t)ap_start);
        while (!cpus[i].start);
    }
}
