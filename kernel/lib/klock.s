    .global fetch_and_add
    .type   fetch_and_add, @function
    .text
fetch_and_add:
    movl $1, %eax
    movl 4(%esp), %ecx
    xadd %eax, (%ecx)
    ret


    .global test_and_set
    .type   test_and_set, @function
    .text
test_and_set:
    movl $1,%eax
    movl 4(%esp),%ecx
    xchg  %eax  ,(%ecx)
    ret