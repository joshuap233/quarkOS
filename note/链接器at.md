为了将内核映射到 3G以上,改了下链接器脚本,有个地方无法立即,这是链接器脚本:



```txt
/* 入口, 跳转到 _start 处执行*/
ENTRY(_start)

/* 输出节 */
SECTIONS
{
	/* .text 从内存 1M 开始*/
	. = 1M;

    init_text_start = .;
    .init.text BLOCK(4K) : ALIGN(4K)
    {
            *(.init.text)
    }

    init_data_start = .;
    .init.data BLOCK(4K) : ALIGN(4K)
    {
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
		*(COMMON)
		*(.bss)
		_ebss = .;
	}

    /* 用于获取内核结束地址 */
	_endKernel = ALIGN(4k);
}
```

我无法理解 `AT (ADDR (.bss) - 0xC0000000)` 的作用,



下面是带有 `AT (ADDR (.bss) - 0xC0000000)` 编译后,objdump 的结果



```txt
❯ i686-elf-objdump -h quarkOS.bin

quarkOS.bin:     file format elf32-i386

Sections:
Idx Name          Size      VMA       LMA       File off  Algn
  0 .init.text    00000151  00100000  00100000  00001000  2**12
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  1 .init.data    00001010  00101000  00101000  00002000  2**12
                  CONTENTS, ALLOC, LOAD, DATA
  2 .text         0000d96e  c0103000  00103000  00004000  2**4
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  3 .rodata       00001535  c0111000  00111000  00012000  2**5
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  4 .eh_frame     000000b0  c0112538  00112538  00013538  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  5 .data         00003082  c0113000  00113000  00014000  2**12
                  CONTENTS, ALLOC, LOAD, DATA
  6 .bss          000060d4  c0117000  00117000  00017082  2**12
                  ALLOC
  7 .debug_info   00016283  00000000  00000000  00017082  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS
  8 .debug_abbrev 0000508d  00000000  00000000  0002d305  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS
  9 .debug_aranges 00000608  00000000  00000000  00032398  2**3
                  CONTENTS, READONLY, DEBUGGING, OCTETS
 10 .debug_ranges 00000108  00000000  00000000  000329a0  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS
 11 .debug_line   00009621  00000000  00000000  00032aa8  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS
 12 .debug_str    00002efd  00000000  00000000  0003c0c9  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS
 13 .comment      00000012  00000000  00000000  0003efc6  2**0
                  CONTENTS, READONLY
 14 .debug_frame  00002bf0  00000000  00000000  0003efd8  2**2
                  CONTENTS, READONLY, DEBUGGING, OCTETS
 15 .debug_loc    00000d87  00000000  00000000  00041bc8  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS

```

下面是去除  `AT (ADDR (.bss) - 0xC0000000)` 的结果

```
❯ i686-elf-objdump -h quarkOS.bin

quarkOS.bin:     file format elf32-i386

Sections:
Idx Name          Size      VMA       LMA       File off  Algn
  0 .init.text    00000151  00100000  00100000  00001000  2**12
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  1 .init.data    00001010  00101000  00101000  00002000  2**12
                  CONTENTS, ALLOC, LOAD, DATA
  2 .text         0000d96e  c0103000  c0103000  00004000  2**12
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  3 .rodata       00001535  c0111000  c0111000  00012000  2**12
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  4 .eh_frame     000000b0  c0112538  c0112538  00013538  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  5 .data         00003082  c0113000  c0113000  00014000  2**12
                  CONTENTS, ALLOC, LOAD, DATA
  6 .bss          000060d4  c0117000  c0117000  00017082  2**12
                  ALLOC
  7 .debug_info   00016283  00000000  00000000  00017082  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS
  8 .debug_abbrev 0000508d  00000000  00000000  0002d305  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS
  9 .debug_aranges 00000608  00000000  00000000  00032398  2**3
                  CONTENTS, READONLY, DEBUGGING, OCTETS
 10 .debug_ranges 00000108  00000000  00000000  000329a0  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS
 11 .debug_line   00009621  00000000  00000000  00032aa8  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS
 12 .debug_str    00002efd  00000000  00000000  0003c0c9  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS
 13 .comment      00000012  00000000  00000000  0003efc6  2**0
                  CONTENTS, READONLY
 14 .debug_frame  00002bf0  00000000  00000000  0003efd8  2**2
                  CONTENTS, READONLY, DEBUGGING, OCTETS
 15 .debug_loc    00000d87  00000000  00000000  00041bc8  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS

```

区别在于 .txt .rodata .data .bss 的 lma 比之前的大 0xC0000000



在网上查找 


- VMA (virtual memory address): 当output file 运作时，section 会得到这个address。
- LMA (load memory address): 当section 被载入(loaded) 时，会放置到这个address。

也就是说删除 0xC0000000 后,内核会被载入到 3G,但是这时候还没有开启虚拟内存,所以会被载入到物理内存 3G 处,很明显物理内存不足会报错...........额以后补充

也就是内核装在在 < 3G 的物理空间,但运行在 >3G 的虚拟内存中

https://www.crifan.com/detailed_lma_load_memory_address_and_vma_virtual_memory_address/