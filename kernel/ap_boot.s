    .code16
    .global ap_start
    .section .ap
    .type ap_start, @function
ap_start:
    cli

    xorw    %ax,%ax
    movw    %ax,%ds
    movw    %ax,%es
    movw    %ax,%ss

    lgdt    gdtr
    movl    %cr0, %eax
    orl     $1, %eax
    movl    %eax, %cr0

    jmpl    $0x08,$ap_start32

# 临时 gdt
.align 8
gdt:
    .long 0, 0
    .long 0xffff, 0xcf9a00    # code
    .long 0xffff, 0xcf9200    # data
gdtr:
    .word 23
    .long gdt

.global init_struct
init_struct:
cr3:
    .long   0
stack_bottom:
    .long   0


.code32
ap_start32:
    movw $0x10,%ax # 0x10 为数据段选择子
    movw %ax,%ds
    movw %ax,%es
    movw %ax,%fs
    movw %ax,%gs
    movw %ax,%ss


    movl cr3, %eax
    movl %eax, %cr3

    movl %cr0, %eax
    orl  $0x80000000, %eax
    mov  %eax, %cr0

    mov stack_bottom, %esp
    call ap_main

	cli
1:	hlt
	jmp 1b

.global ap_end
ap_end:
