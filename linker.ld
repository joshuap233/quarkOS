/* 入口, 跳转到 _start 处执行*/
ENTRY(_start)

/* 输出节 */
SECTIONS
{
	/* .text 从内存 1M 开始*/
	. = 1M;

    .init.text ALIGN(4K) :
    {
        init_text_start = .;
        *(.init.multiboot2)
        *(.init.text)
    }

    .init.data ALIGN(4K) :
    {
        init_data_start = .;
        *(.init.data)
    }

    . += 0xC0000000;

	/* 读,执行代码 */
	.text ALIGN(4K) : AT(ADDR (.text) - 0xC0000000)
	{
		_startKernel = .;
		*(.text)
	}

	/* 只读数据 */
	.rodata ALIGN(4K) : AT(ADDR (.rodata) - 0xC0000000)
	{
	    _rodataStart = .;
		*(.rodata)
	}

	/* 可读写数据 */
	.data ALIGN(4K) : AT(ADDR (.data) - 0xC0000000)
	{
	    _dataStart = .;
		*(.data)
	}

	/* 未初始化的可读写 数据/栈 */
	.bss ALIGN(4K) : AT (ADDR (.bss) - 0xC0000000)
	{
	    _bssStart = .;
		*(COMMON)
		*(.bss)
	}

    /* 用于获取内核结束地址 */
	_endKernel = ALIGN(4k);
}