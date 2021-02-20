    .extern kRelease
    .extern worker
    .extern kthread_exit_

    .global kthread_worker
    .type kthread_worker, @function
    .text
kthread_worker:
    call kRelease