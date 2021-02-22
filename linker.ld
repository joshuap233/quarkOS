/* 入口, 跳转到 _start 处执行*/
ENTRY(_start)

/* 输出节 */
SECTIONS
{
	/* .text 从内存 1M 开始*/
	. = 1M;

	/* 读,执行代码 */
	.text BLOCK(4K) : ALIGN(4K)
	{
		_startKernel = .;
		*(.multiboot2)
		*(.text)
	}

	/* 只读数据 */
	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata)
	}

	/* 可读写数据 */
	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
	}

	/* 未初始化的可读写 数据/栈 */
	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)
		_ebss = .;
	}

    /* 用于获取内核结束地址 */
	_endKernel = ALIGN(4k);
}