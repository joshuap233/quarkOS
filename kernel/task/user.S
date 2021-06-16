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

    .global exec
    .type exec, @function
    .section .user.text
exec:
    mov 4(%esp), %ebx
    mov $SYS_EXEC, %eax
    int $SYS_CALL_NO
    ret