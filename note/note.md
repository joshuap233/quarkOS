我在考虑要不要用 cython 或者 python 调用 c 做测试

一些资料:

- [键盘输入](https://wiki.osdev.org/Keyboard)

- 内存:
  - [Memory_Map](https://wiki.osdev.org/Memory_Map)
  - [Detecting Memory](https://wiki.osdev.org/Detecting_Memory_(x86))
  - [GDT](https://wiki.osdev.org/GDT)
  - [LDT](https://wiki.osdev.org/LDT)
  - [TSS 任务状态段](https://wiki.osdev.org/TSS)
  
- 网络:
  
  - [网络协议栈](https://wiki.osdev.org/Network_Stack)
- [TCP 实现](https://zhuanlan.zhihu.com/p/175998415)
  
- 进程:
  - [上下文切换](https://wiki.osdev.org/Context_Switching#Hardware_Context_Switching)
  - [进程与线程管理](https://wiki.osdev.org/Processes_and_Threads)

- 显示:
  - 开启图形模式:

    - https://wiki.osdev.org/How_do_I_set_a_graphics_mode
    - 或者查看 multiboot 标准,grub可以开启: https://wiki.osdev.org/How_do_I_set_a_graphics_mode
  - 文本显示
    - VGA(VGA 模式已经过时了,显卡厂商放弃了 VGA 兼容,转而使用 GOP,但部分虚拟机模拟器支持)
      - [文本模式](https://wiki.osdev.org/Text_mode)
    - PC Screen Font
    - [GOP](https://wiki.osdev.org/GOP)
  - [GUI](https://wiki.osdev.org/GUI)
  
- 中断:

  - [IVT 中断向量表](https://wiki.osdev.org/IVT)

- 硬件:
  - 寄存器:
    - [x86寄存器](https://wiki.osdev.org/CPU_Registers_x86)
    - [CPU Registers x86](https://wiki.osdev.org/CR0#CR0)
  - [CPUID cpu 信息检索](https://wiki.osdev.org/CPUID)

    - [SSE 扩展指令](https://wiki.osdev.org/SSE)
  - [第21根地址线](https://wiki.osdev.org/A20_Line)
  
- 文件系统
  
  - [文件系统](https://wiki.osdev.org/File_System)
  
- 启动:
  - [Multiboot Specification version 0.6.96](https://www.gnu.org/software/grub/manual/multiboot/multiboot.html)
  - [multiboot2](https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html#Examples)
  - [GRUB](https://wiki.osdev.org/GRUB)
  
- 工具:
  - [qemu](https://wiki.osdev.org/Qemu)
  - 跨平台编译器:
    - [why](https://wiki.osdev.org/Why_do_I_need_a_Cross_Compiler%3F#Options_you_should_link_with)
    - [how to](https://wiki.osdev.org/GCC_Cross-Compiler)
  - ELF 可执行文件

    - [elf 文档](https://wiki.osdev.org/ELF)
    - [elf 教程](https://wiki.osdev.org/ELF_Tutorial)
  - [Linker 脚本](https://wiki.osdev.org/Linker_Scripts)
  - [AT&T 风格 as 汇编](https://docs.huihoo.com/gnu_linux/own_os/preparing-asm_4.htm)
  - [Executable Formats](https://wiki.osdev.org/Executable_Formats)
  - [GCC 编译的背后](http://tinylab.org/behind-the-gcc-compiler/#%E7%AE%80%E8%BF%B0-2)

- 内联汇编

  - https://wiki.osdev.org/Inline_Assembly
  - [内联汇编示例,有io,中断使用示例](https://wiki.osdev.org/Inline_Assembly/Examples#LIDT)
  - [GCC-Inline-Assembly-HOWTO](https://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html)
  - [GCC-Inline-Assembly-HOWTO-翻译](https://www.cnblogs.com/cposture/p/9029043.html#%E5%8E%9F%E6%96%87%E9%93%BE%E6%8E%A5%E4%B8%8E%E8%AF%B4%E6%98%8E)
  - [文档(晦涩难懂)](https://gcc.gnu.org/onlinedocs/gcc/Using-Assembly-Language-with-C.html#Using-Assembly-Language-with-C)

- [debug](https://wiki.osdev.org/How_Do_I_Use_A_Debugger_With_My_OS)

- [BIOS](https://wiki.osdev.org/BIOS)

- [如何写一个控制台教程](http://www.osdever.net/bkerndev/Docs/printing.htm)

- [端口列表](https://wiki.osdev.org/Can_I_have_a_list_of_IO_Ports)

- [System V ABI](https://wiki.osdev.org/System_V_ABI)

- [搭建操作系统骨架以及进阶设计](https://wiki.osdev.org/Bare_Bones#Frequently_Asked_Questions)

- [Going Further on x86](https://wiki.osdev.org/Going_Further_on_x86)

- [c Freestanding](https://wiki.osdev.org/C_Library#Freestanding_and_Hosted)
  - [头文件详细信息](https://wiki.osdev.org/Implications_of_writing_a_freestanding_C_project)
  - Freestanding 下可用的一些头文件:  
    - <float.h>    

      - 定义了浮点类型的一些极限值
      - https://zh.wikipedia.org/wiki/Float.h

    - <iso646.h>   

      - https://zh.wikipedia.org/wiki/C%E6%9B%BF%E4%BB%A3%E6%A0%87%E8%AE%B0

      - 允许使用 and、or 等宏(相当于 &&、||等)

    - <limits.h>    

      - https://zh.wikipedia.org/wiki/Limits.h
      - 定义了整数类型的一些极限值

    - <stdalign.h>

      - https://zh.wikipedia.org/wiki/Stdalign.h
      - 用于对齐

    - <stdarg.h>

      - https://zh.wikipedia.org/wiki/Stdarg.h
      - 让函数能够接收不定量参数

    - <stdbool.h>

      -  bool 类型，相当于 _Bool, true,false
      - https://zh.wikipedia.org/wiki/Stdbool.h

    - <stddef.h>  

      - ptrdiff_t 有符号整数型
      - size_t 无符号整数型
      - wchar_t 16位或32位整数型
      - NULL 与实现相关的空指针的值
      - offsetof(type, member-designator) 结构体内成员的偏移量
      - https://zh.wikipedia.org/wiki/Stddef.h

    - <stdint.h>  

      - https://zh.wikipedia.org/zh-hans/Stdint.h
      - 定义了具有特定位宽的整型，以及对应的宏；还列出了在其他标准头文件中定义的整型的极限。
      - untx_t 、uintx_t
  
- <stdnoreturn.h>

- 资料里有一些 c 库,但我不但算用,因为我认为会增加学习的复杂度 :-( 

- 参考文献(可能会用到):

  - [Developing Your Own OS On IBM PC](https://docs.huihoo.com/)
  - [从零开始的 OS 轮子——保护模式](https://xr1s.me/2018/04/07/os-from-scratch-protected-mode/)
  - [一个小内核实现](http://wiki.0xffffff.org/)
  - [xv6](https://github.com/mit-pdos/xv6-public)
  - [How-to-Make-a-Computer-Operating-System](https://github.com/SamyPesse/How-to-Make-a-Computer-Operating-System)
  - 《x86 汇编语言 从实模式到保护模式》
  - 《汇编语言 第四版》
  - 《操作系统导论》
  - 《C Primer Plus》
  - 《Unix 环境高级编程》

  

1.bss text data 段
参考:



> An executable format is the file format created by the compiler and linker

> Definitions
  TEXT is the actual executable code area,
  DATA is "initialized" data,
  BSS is "un-initialized" data.
  The BSS (Block Started by Symbol) needn't to be present in an executable file. At load-time, the loader will still allocate memory for it and wipes this memory with zeros (this is assumed by C programs, for instance).

2.编译与链接:

```shell script
gcc -S hello.c # 生成汇编文件

# 目标文件需要用 ld 进一步进行链接生成可执行文件或共享库
gcc -c hello.s # 将汇编文件编译成目标代码
as -o hello.o hello.s   #用as把汇编语言编译成目标代码

file hello.o   # file命令用来查看文件类型

```



gcc 内部也要将 C 代码经过编译(翻译成汇编)、汇编(生成目标文件)、链接(生成可执行文件)三个阶段。



一些名词缩写:

- [PIC/PIE 地址无关代码/地址无关可执行文件](https://zh.wikipedia.org/wiki/%E5%9C%B0%E5%9D%80%E6%97%A0%E5%85%B3%E4%BB%A3%E7%A0%81)
  - PIC广泛使用于共享库，使得同一个库中的代码能够被加载到不同进程的地址空间中。


debug 的时候记得关掉 gcc 优化,开了优化之后, assert 之前的打印语句失效了(如果断言失败)



中断号:

https://wiki.osdev.org/Exceptions#General_Protection_Fault



内联汇编示例:

https://wiki.osdev.org/Inline_Assembly/Examples#IO_WAIT