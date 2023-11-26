text 段,代码段

data段, 静态变量,全局变量(已初始化),如果变量在函数内部声明,则存储在本地堆栈中

bss段, 静态变量,全局变量(未初始化或被初始化为0),

heap ,堆区域一般在 bss 段结尾,向高地址增长:

> The heap area is managed by [malloc](https://en.wikipedia.org/wiki/Malloc), calloc, realloc, and free, which may use the [brk](https://en.wikipedia.org/wiki/Sbrk) and [sbrk](https://en.wikipedia.org/wiki/Sbrk) system calls to adjust its size
>
> The heap area is shared by all threads, shared libraries, and dynamically loaded modules in a process.



stack 栈区域一般在高低值并且向低地址增长

> 为一个函数调用推入的一组值称为“堆栈帧”。 堆栈帧至少包含一个返回地址。

自动变量也保存在栈中

(8086 中 ss:sp 指向栈顶)



![img](image/page1-149px-Program_memory_layout.pdf.jpg)

参考:https://en.wikipedia.org/wiki/Data_segment