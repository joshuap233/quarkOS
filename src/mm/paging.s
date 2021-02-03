    .global cr3_set
    .type   cr3_set, @function
    .text
cr3_set:
    mov  4(%esp),%eax
    mov  %eax, %cr3

    /* 开启分页 */
    mov %cr0, %eax
    or  $0x80000000, %eax
    mov %eax, %cr0
    ret