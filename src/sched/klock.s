    .global k_unlock
    .type   k_unlock, @function
    .text
k_unlock:
    sti
    ret

    .global k_lock
    .type   k_lock, @function
    .text
k_lock:
    cli
    ret


    .global spinlock_lock
    .type   spinlock_lock, @function
    .text
spinlock_lock:
    movl  $1,%eax

    xchg  %eax,4(%esp)
    cmpl  $1,%eax
    jne   2f

1:  pause
    xchg  %eax,4(%esp)
    cmpl  $1,%eax
    je    1b
2:  ret



    .global fetch_and_add
    .type   fetch_and_add, @function
    .text
fetch_and_add:
    movl $1, %eax
    movl 4(%esp), %ecx
    xadd %eax, (%ecx)
    ret
