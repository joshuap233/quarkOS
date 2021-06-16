#include <syscall/syscall.h>
    .global fork
    .type fork, @function
    .section .user.text
fork:
    mov $SYS_FORK, %eax
    int $SYS_CALL_NO
    ret


    .global exit
    .type exit, @function
    .section .user.text
exit:
    mov 4(%esp), %ebx
    mov $SYS_EXIT, %eax
    int $SYS_CALL_NO
    ret


    .global gets
    .type gets, @function
    .section .user.text
gets:
    mov 4(%esp), %ebx
    mov 8(%esp), %ecx
    mov $SYS_GETCHAR, %eax
    int $SYS_CALL_NO
    ret

    .global puts
    .type puts, @function
    .section .user.text
puts:
    mov 4(%esp), %ebx
    mov 8(%esp), %ecx
    mov $SYS_PUTCHAR, %eax
    int $SYS_CALL_NO
    ret

    .global exec
    .type exec, @function
    .section .user.text
exec:
    mov 4(%esp), %ebx
    mov $SYS_EXEC, %eax
    int $SYS_CALL_NO
    ret

    .global cls
    .type cls, @function
    .section .user.text
cls:
    mov $SYS_CLS, %eax
    int $SYS_CALL_NO
    ret