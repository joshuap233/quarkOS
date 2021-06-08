    .global switch_to
    .type switch_to, @function
    .text
switch_to:
    movl 4(%esp),%eax # 需要保存的上下文指针地址
    movl 8(%esp),%ecx # 下一个线程上下文指针地址

    # call switch_to 会自动压入返回地址(eip)
    push %edi
    push %esi
    push %ebp
    push %ebx
    pushf

    movl %esp, (%eax) # 保存 context 地址

    # 任务切换
    movl %ecx, %esp

    popf
    pop  %ebx
    pop  %ebp
    pop  %esi
    pop  %edi

    ret
