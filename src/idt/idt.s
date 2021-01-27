    .global idtr_set
    .text
idtr_set:
    movl 4(%esp),%eax
    lidt (%eax)
    ret