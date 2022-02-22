#include <syscall/syscall.h>
    .global fork
    .type fork, @function
    .text
fork:
    mov $SYS_FORK, %eax
    int $SYS_CALL_NO
    ret


    .global exit
    .type exit, @function
    .text
exit:
    mov 4(%esp), %ebx
    mov $SYS_EXIT, %eax
    int $SYS_CALL_NO
    ret


    .global gets
    .type gets, @function
    .text
gets:
    mov 4(%esp), %ebx
    mov 8(%esp), %ecx
    mov $SYS_GETCHAR, %eax
    int $SYS_CALL_NO
    ret

    .global puts
    .type puts, @function
    .text
puts:
    mov 4(%esp), %ebx
    mov 8(%esp), %ecx
    mov $SYS_PUTCHAR, %eax
    int $SYS_CALL_NO
    ret

    .global exec
    .type exec, @function
    .text
exec:
    mov 4(%esp), %ebx
    mov $SYS_EXEC, %eax
    int $SYS_CALL_NO
    ret

    .global cls
    .type cls, @function
    .text
cls:
    mov $SYS_CLS, %eax
    int $SYS_CALL_NO
    ret

    .global sbrk
    .type sbrk, @function
    .text
sbrk:
    mov 4(%esp), %ebx
    mov $SYS_SBRK, %eax
    int $SYS_CALL_NO
    ret