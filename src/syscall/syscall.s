    # 进入用户模式前初始化
    .extern  syscall_isr

    .global  syscall_entry
    .type    syscall_entry, @function
    .text
syscall_entry:
    # cpu 会自动保存 (ss,esp),eflags,cs,eip
    push %gs
    push %fs
    push %es
    push %ds

    # esp 压栈记录 sys_context 位置
    push %esp

    # 用于传递参数, eax 传递需要调用的函数编号
    push %ebp
    push %edi
    push %esi
    push %edx
    push %ecx
    push %ebx
    push %eax

    call syscall_isr

    add  $32, %esp
    jmp 1f

    .global  syscall_ret
    .type    syscall_ret, @function
    .text
syscall_ret:
    movl %ebx,%eax
1:
    pop  %ds
    pop  %es
    pop  %fs
    pop  %gs

    iret