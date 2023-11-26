CPU 取指 -> 指令放入指令缓冲区 -> ip +1  -> 执行指令

中间两个步骤的顺序我不确定??





CDECL stack

https://wiki.osdev.org/Stack



栈结构

![2021-02-20 22-04-26 的屏幕截图](image/2021-02-20%2022-04-26%20%E7%9A%84%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE.png)





\-\-\-\-\-\-\-\-\-    高地址

返回地址

\-\-\-\-\-\-\-\-\-    

调用函数的 ebp

\-\-\-\-\-\-\-\-\-    <- ebp 

第一个本地变量

\-\-\-\-\-\-\-\-\-   低地址



ebp 并不是指向栈帧顶, 访问局部变量通过 `-4(%ebp)` `-8(%ebp)`的方式访问









下面是 x86 代码:

```assembly
	.file	"test.c"
	.text
	.globl	test
	.type	test, @function
test:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$16, %esp
	
	# 处理本地变量
	movl	$0, -4(%ebp)
	addl	$1, -4(%ebp)
	addl	$1, 8(%ebp) 
    
	movl	8(%ebp), %eax # 保存返回值到 eax
	leave   
	# leave 分为两步:
	# pop %ebp (进入函数时将 ebp 压栈)
    # mov %ebp,%esp  
	
	ret
	.size	test, .-test
	
	.globl	main
	.type	main, @function
main:
	# main 是 caller 也是 callee
	# ebp 压栈, esp 加载到 ebp
	pushl	%ebp
	movl	%esp, %ebp
	#设置 ebp esp 间可用空间为 16 字节
	subl	$16, %esp
	
	# 处理本地变量
	movl	$0, -4(%ebp)
	addl	$1, -4(%ebp)
	
	# 参数压栈
	pushl	-4(%ebp)
	call	test
	
	addl	$4, %esp 
	# test 函数 leave 指令将 ebp 加载到 esp, 
	# esp 加上 test 函数移动的 ebp 次数 * 4 即为 原来的 esp,
	# call 返回前 (ebp指向返回地址, ebp +4 指向参数)
	movl	$0, %eax # 返回值
	leave
	ret
	.size	main, .-main
	.ident	"GCC: (GNU) 10.2.0"
```

这是对应的 c 代码:

```c
int test(int i){
  int j = 0;
  j++;
  i++;
  return i;
}

int main(){
	int i =0;
	i++;
	test(i);
	return 0;
}
```









见 x86 enter leave 指令

caller(调用者) 需要做的事情:

- 将 callee 参数从右到做压栈
- 执行 call 指令
- 将局部变量弹栈,或者简单地移动 esp,忽略参数



callee (被调用的例程) 需要做的事情

- 将 caller 的 ebp 压栈(栈帧指针)
- 将当前 ebp 设置为 esp
- 将本地数据存储到栈上
- 弹出所有本地数据退出,或者将 ebp 加载到 esp 快速退出
- 弹出 ebp 并返回, 将返回值存储在 eax 中

c 编译器会自动做这些事情,但如果使用 c/c++ 调用汇编, 或者汇编调用 c/c++,不会有上面的部分,

比如 c 调用汇编, 调用者不会有: `addl	$4, %esp `, 而汇编代码保留原样 



添加 ``-fomit-frame-pointer` 参数后翻译(源代码与上面的相同)(发现 ebp 没了):

```asm
	.file	"test.c"
	.text
	.globl	test
	.type	test, @function
test:
	subl	$16, %esp
	movl	$0, 12(%esp)
	addl	$1, 12(%esp)
	addl	$1, 20(%esp)
	movl	20(%esp), %eax
	addl	$16, %esp
	ret
	.size	test, .-test
	.globl	main
	.type	main, @function
main:
	subl	$16, %esp
	movl	$0, 12(%esp)
	movl	$0, 8(%esp)
	addl	$1, 12(%esp)
	pushl	12(%esp)
	call	test
	addl	$4, %esp
	addl	$1, 12(%esp)
	addl	$1, 8(%esp)
	movl	$0, %eax
	addl	$16, %esp
	ret
	.size	main, .-main
	.ident	"GCC: (GNU) 10.2.0"

```



使用 -fno-omit-frame-pointer 可以使用 ebp 

添加 -O1 参数也会去掉栈帧



如果 test 函数 没有返回值,则 `movl	8(%ebp), %eax` 会 替换为 `nop`, 所以者万一有什么用吗?



### Security

The stack is easy to use, but it has one problem. There is no "end," so it is vulnerable to a variation of the buffer overflow attack. The attacker pushes more elements than the stack is able to contain, so elements are pushed outside the stack memory, overwriting code, which the attacker can then execute.

In X86 protected mode it can be solved by allocating a [GDT descriptor](https://wiki.osdev.org/GDT) solely for the stack, which defines its boundaries.

