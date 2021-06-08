//
// Created by pjs on 2021/6/7.
//
#include <userspace/lib.h>
#include <types.h>
#include <syscall/syscall.h>

SECTION(".user.text")
int fork() {
    register int eax asm("eax");

    asm volatile(
    "mov %0, %%eax\n\t"
    "int $0x80"
    ::"rI"(SYS_FORK)
    );
    return eax;
}

SECTION(".user.text")
int exit(UNUSED int errno) {
    register int eax asm("eax");

    asm volatile(
    "mov %0, %%eax\n\t"
    "int $0x80"
    ::"rI"(SYS_EXIT)
    );
    return eax;
}