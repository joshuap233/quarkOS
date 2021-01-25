    .global gdtr_set
    .text
gdtr_set:
    movl 4(%esp),%eax
    lgdt (%eax)
    movw $0x10,%ax # 0x10 为数据段选择子
    movw %ax,%ds
    movw %ax,%es
    movw %ax,%fs
    movw %ax,%gs
    movw %ax,%ss
    jmp $0x08,$foo # 设置 cs 为 0x08(代码段选择子),并清空流水线

foo:
    ret