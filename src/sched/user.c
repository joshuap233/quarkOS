//
// Created by pjs on 2021/3/11.
//

#include <sched/user.h>
#include <sched/task.h>

extern void userspace_init();

int user_test() {
    int i = 1 + 2;
    asm volatile(
    "mov %0, %%eax\n\t"
    "int $0x80"
    ::"r"(i)
    );
    return i;
}

void goto_usermode() {
    userspace_init();
}