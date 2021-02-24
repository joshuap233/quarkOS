    .extern kthread_exit

    .global kthread_worker
    .type   kthread_worker, @function
    .text
kthread_worker:
    movl  4(%esp), %eax
    pushl 8(%esp)
    call  *%eax
    call  kthread_exit
    ret