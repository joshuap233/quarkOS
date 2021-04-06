    # 进入用户模式前初始化
    .type userspace_init, @function
    .text
userspace_init:
    movw  $0x23, %gs # 用户数据段
    movw  $0x23, %fs
    movw  $0x23, %es
    movw  $0x23, %ds


    pushf %esp
    pushf
    pushl $0x1b   # 用户代码段
    pushl user_test
    iret


    .type   trap_ret, @function
    .text
trap_ret:
    popw %gs
    popw %fs
    popw %es
    popw %ds

    iret