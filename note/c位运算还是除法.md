我用的 i686-elf-gcc



```c
int func(int a){ 
  return  a >>2;	 
}
```

对应的汇编:



```asm

	.file	"test.c"
	.text
	.globl	func
	.type	func, @function
func:
	pushl	%ebp
	movl	%esp, %ebp
	movl	8(%ebp), %eax
	sarl	$2, %eax
	popl	%ebp
	ret
	.size	func, .-func
	.ident	"GCC: (GNU) 10.2.0"

```





```c
int func(int a){ 
  return  a / 4 ;	 
}
```

对应的汇编:



```asm
	.file	"test.c"
	.text
	.globl	func
	.type	func, @function
func:
	pushl	%ebp
	movl	%esp, %ebp
	movl	8(%ebp), %eax
	leal	3(%eax), %edx
	testl	%eax, %eax
	cmovs	%edx, %eax
	sarl	$2, %eax
	popl	%ebp
	ret
	.size	func, .-func
	.ident	"GCC: (GNU) 10.2.0"

```

