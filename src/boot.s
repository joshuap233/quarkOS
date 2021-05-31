    .extern kern_entry
    /* multiboot 头常量 */
    .set MAGIC,         0xE85250D6
    .set ARCHITECTURE,  0
    .set HEADER_LENGTH, headerEnd - headerStart
    .set CHECKSUM,      -(HEADER_LENGTH + MAGIC + ARCHITECTURE)

/* multiboot2 头需要 8 字节对齐,且必须在 OS image 的前 1K */
    .section .init.text
    .align 8
headerStart:

    .long MAGIC
    .long ARCHITECTURE
    .long HEADER_LENGTH
    .long CHECKSUM
/*一系列 tag*/

/* 请求信息 */
    .short 1
    .short 0
    .long 8+reqinfoEnd-reqinfoStart

reqinfoStart:
    .long 6  # memory map
    .long 10 # APM 表
reqinfoEnd:


    /*页对齐*/
    .short 6
    .short 1
    .long 8

    /*终结 tag,需要按照 multiboot2 规范填充*/
    .short 0
    .short 0
    .long 8
headerEnd:
/*  multiboot2 结束  */

/*
* linker 脚本指定 _start symbol 为内核开始处,bootloader 会跳转到这里
* .global 使 ld 脚本可以看到 _start symbol
*/
    .section .init.text
    .global _start
    .type _start, @function
_start:

	/* 设置临时内核栈, x86 栈向低地址扩展, c 函数执行需要栈 */
	mov $stack_top, %esp

    movl %eax, magic
    movl %ebx, mba
	call kern_entry

	cli
1:	hlt
	jmp 1b

.size _start, . - _start


/* 内核解析 multiboot info 相关信息的全局数据 */
    .section .data
    .global mba
mba:
    .long 0

    .section .data
    .global magic
magic:
    .long 0


/* 临时内核栈 */
    .section .init.data
    .align   16
stack_bottom:
    .skip    4096
stack_top:


/* 临时页目录,放在 bss 段,内存置 0 */
    .section .init.bss
    .global boot_page_dir
    .global boot_page_table1
    .global boot_page_table2
    .align   4096
boot_page_dir:
    .skip    4096
boot_page_table1:
	.skip 4096
boot_page_table2:
	.skip 4096
