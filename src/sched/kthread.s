    .extern k_lock_release
    .extern kthread_exit_

    .global kthread_worker
    .type   kthread_worker, @function
    .text
kthread_worker:
    call  k_lock_release
    pushl 8(%esp)
    movl  8(%esp), %eax
    call  *%eax
    pushl 16(%esp)
    call  kthread_exit_
    ret