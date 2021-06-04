//
// Created by pjs on 2021/3/11.
//

#include <sched/user.h>
#include <sched/kthread.h>

extern void userspace_init();

int user_test() {
    int i = 1 + 2;
    return i;
}

int goto_usermode() {
    userspace_init();
}