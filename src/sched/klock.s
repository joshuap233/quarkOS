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
    jmp   .condition
.loop:
    pause
.condition:
    xchg  %eax,4(%esp)
    cmpl  $1,%eax
    je    .loop
    ret
