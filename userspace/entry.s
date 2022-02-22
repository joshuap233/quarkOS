    .extern main
    .extern exit

    .global _start
    .type _start, @function
    .text
_start:
    call main
    push %eax
    call exit
    ret