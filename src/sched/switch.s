    .global switch_to
    .type switch_to, @function
    .text
switch_to:
    # 存储上下文
    movl 4(%esp),%eax # 需要保存的上下文首地址
    movl 8(%esp),%ecx # 下一个线程上下文首地址
    movl %esp,%edx

    add  $24,%eax     # eax 为 context 尾地址
    movl %eax, %esp
    push %edi
    push %esi
    push %ebp
    push %ebx
    pushf
    push %edx      # 保存 esp

    # 任务切换
    movl %ecx, %esp

    pop  %edx   # 保存 esp
    popf
    pop  %ebx
    pop  %ebp
    pop  %esi
    pop  %edi

    mov  %edx, %esp

    ret
