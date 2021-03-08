    .global idtr_set
    .type idtr_set, @function
    .text
idtr_set:
    movl 4(%esp),%eax
    lidt (%eax)
    ret