//
// Created by pjs on 2021/6/5.
//
//
#include <sched/user.h>
#include <sched/task.h>
#include <userspace/console.h>
#include <types.h>


SECTION(".user.text")
int user_init() {
    int i = 1 + 2;
//    asm volatile(
//    "mov %0, %%eax\n\t"
//    "int $0x80"
//    ::"r"(i)
//    );

    return i;
}
