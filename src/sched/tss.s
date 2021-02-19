    .global tr_set
    .type gdtr_set, @function
    .text
tr_set:
    movw 4(%esp),%ax
    ltr  %ax
    ret
