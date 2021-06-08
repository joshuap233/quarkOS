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

    push %ebp

    #  eax,ecx,edx,esi,edi传递参数
    push %edi
    push %esi
    push %ecx
    push %ebx
    push %eax

    call syscall_isr

    add  $20, %esp

    pop  %ebp
    pop  %ds
    pop  %es
    pop  %fs
    pop  %gs

    iret