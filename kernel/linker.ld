/* 入口, 跳转到 _start 处执行*/
ENTRY(_start)

/* 输出节 */
SECTIONS
{
	/* .text 从内存 1M 开始*/
	. = 1M;

    /* 初始代码 */
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

    /* 用户空间代码 */
    .user.text ALIGN(4K) :
    {
        user_text_start = .;
        *(.user.text)
    }

    .user.rodata ALIGN(4K) :
    {
        user_rodata_start = .;
        *(.user.rodata)
    }

    .user.data ALIGN(4K) :
    {
        user_data_start = .;
        *(.user.data)
    }

    .user.bss ALIGN(4K) :
    {
        user_bss_start = .;
        *(.user.bss)
    }
    user_bss_end = .;


    /* 内核代码, 注意 elf 的 VMA 与 LMA*/
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