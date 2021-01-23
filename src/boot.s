.extern kernel_main

/* multiboot 头常量 */
.set MAGIC,         0xE85250D6
.set ARCHITECTURE,  0
.set HEADER_LENGTH, headerEnd - headerStart
.set CHECKSUM,      -(HEADER_LENGTH + MAGIC + ARCHITECTURE)

/* multiboot2 头需要 8 字节对齐,且必须在 OS image 的前 1K */

.section .multiboot2
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

/*
The multiboot standard does not define the value of the stack pointer register
(esp) and it is up to the kernel to provide a stack. This allocates room for a
small stack by creating a symbol at the bottom of it, then allocating 16384
bytes for it, and finally creating a symbol at the top. The stack grows
downwards on x86. The stack is in its own section so it can be marked nobits,
which means the kernel file is smaller because it does not contain an
uninitialized stack. The stack on x86 must be 16-byte aligned according to the
System V ABI standard and de-facto extensions. The compiler will assume the
stack is properly aligned and failure to align the stack will result in
undefined behavior.
*/
.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

/*
linker 脚本指定 _start symbol 为内核开始处,bootloader 会跳转到这里
.global 使 ld 脚本可以看到 _start symbol
*/
.section .text
.global _start
.type _start, @function
_start:

	/* 设置内核栈, x86 栈向低地址扩展, c 函数执行需要栈 */
	mov $stack_top, %esp

	/*
	This is a good place to initialize crucial processor state before the
	high-level kernel is entered. It's best to minimize the early
	environment where crucial features are offline. Note that the
	processor is not fully initialized yet: Features such as floating
	point instructions and instruction set extensions are not initialized
	yet. The GDT should be loaded here. Paging should be enabled here.
	C++ features such as global constructors and exceptions will require
	runtime support to work as well.
	*/

	/*
	Enter the high-level kernel. The ABI requires the stack is 16-byte
	aligned at the time of the call instruction (which afterwards pushes
	the return pointer of size 4 bytes). The stack was originally 16-byte
	aligned above and we've pushed a multiple of 16 bytes to the
	stack since (pushed 0 bytes so far), so the alignment has thus been
	preserved and the call is well defined.
	*/
	call kernel_main

	/*
	If the system has nothing more to do, put the computer into an
	infinite loop. To do that:
	1) Disable interrupts with cli (clear interrupt enable in eflags).
	   They are already disabled by the bootloader, so this is not needed.
	   Mind that you might later enable interrupts and return from
	   kernel_main (which is sort of nonsensical to do).
	2) Wait for the next interrupt to arrive with hlt (halt instruction).
	   Since they are disabled, this will lock up the computer.
	3) Jump to the hlt instruction if it ever wakes up due to a
	   non-maskable interrupt occurring or due to system management mode.
	*/
	cli
1:	hlt
	jmp 1b

/*
Set the size of the _start symbol to the current location '.' minus its start.
This is useful when debugging or when you implement call tracing.
*/
.size _start, . - _start
