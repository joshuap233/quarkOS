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

    .global test_and_set
    .type   test_and_set, @function
    .text
test_and_set:
    movl 4(%esp), %ecx
    movl (%ecx) , %eax

    movl %eax   , %edx
    xchg %edx   , 8(%esp)
    movl %edx   , (%ecx)
    ret


