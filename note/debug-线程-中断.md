display /i $pc 

查看当前执行代码的汇编

也可以用 disas 查看汇编代码

disas funcname 查看函数反汇编



gdb 可以使用 watch *mem_addr 跟踪 mem_addr 处的内存变化, 多亏了这玩意,不然多线程 debug 得弄一天

如果使用了 -g 标志编译会跳转到源码,没使用则跳转到汇编, 如果修改没生效整个项目都重新编译下把...粗暴但简单







============gcc 对不起,原来是我自己 switch 函数的锅!!=======================

gcc (__attribute__((interrupt)))这个蠢东西越过的栈底访问数据:\





```asm
isr33:
	push   %ebp
	mov    %esp,%ebp
	push   %ecx
	push   %edx
	push   %eax
	and    $0xfffffff0,%esp
	sub    $0x30,%esp
	lea    0x4(%ebp),%eax
	mov    %eax,0x1c(%esp)  # 就是这里,蠢东西....debug找了一天
	movw   $0x60,0x2a(%esp)  
	movzwl 0x2a(%esp),%eax
	mov    %eax,%edx
	in     (%dx),%al
	mov    %al,0x29(%esp)
	movzbl 0x29(%esp),%eax
	nop
	movzbl %al,%eax
	sub    $0xc,%esp
	push   %eax
	cld
	call   0x102221                   ; <kb_sc_parse>
	add    $0x10,%esp
	movw   $0x20,0x2e(%esp)
	movb   $0x20,0x2d(%esp)
	movzwl 0x2e(%esp),%edx
	movzbl 0x2d(%esp),%eax
	out    %al,(%dx)
	nop
	nop
	nop
	lea    -0xc(%ebp),%esp
	pop    %eax
	pop    %edx
	pop    %ecx
	pop    %ebp
	iret
```

