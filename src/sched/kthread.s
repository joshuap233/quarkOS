    .extern kthread_exit

    .global kthread_worker
    .type   kthread_worker, @function
    .text
kthread_worker:
    xorl  %ebp, %ebp
    movl  4(%esp), %eax
    pushl 8(%esp)
    sti
    call  *%eax
    call  kthread_exit
    ret