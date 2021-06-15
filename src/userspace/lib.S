#include <syscall/syscall.h>
    .global fork
    .type fork, @function
fork:
    mov $SYS_FORK, %eax
    int $SYS_CALL_NO
    ret


    .global exit
    .type exit, @function
exit:
    mov 4(%esp), %ebx
    mov $SYS_EXIT, %eax
    int $SYS_CALL_NO
    ret


    .global gets
    .type gets, @function
gets:
    mov 4(%esp), %ebx
    mov 8(%esp), %ecx
    mov $SYS_GETCHAR, %eax
    int $SYS_CALL_NO
    ret

    .global puts
    .type puts, @function
puts:
    mov 4(%esp), %ebx
    mov 8(%esp), %ecx
    mov $SYS_PUTCHAR, %eax
    int $SYS_CALL_NO
    ret

    .global exec
    .type exec, @function
exec:
    mov 4(%esp), %ebx
    mov $SYS_EXEC, %eax
    int $SYS_CALL_NO
    ret