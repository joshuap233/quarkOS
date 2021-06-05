
    # 进入用户模式前初始化
    .extern user_test
    .global  userspace_init
    .type userspace_init, @function
    .text
userspace_init:
    # cli
    movw  $0x23, %ax
    movw  %ax,   %gs # 用户数据段
    movw  %ax,   %fs
    movw  %ax,   %es
    movw  %ax,   %ds

    movl   %esp, %eax

    pushl  $0x23
    pushl  %eax
    pushfl
    pushl  $0x1b      # 用户代码段
    pushl  $user_test
    iret


    .extern task_exit
    .global thread_entry
    .type   thread_entry, @function
    .text
thread_entry:
    xorl  %ebp, %ebp
    movl  4(%esp), %eax
    pushl 8(%esp)
    sti
    call  *%eax
    call  task_exit
    ret
