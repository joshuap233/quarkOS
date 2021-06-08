//
// Created by pjs on 2021/6/5.
//
//
#include <sched/user.h>
#include <sched/task.h>
#include <userspace/console.h>
#include <types.h>
#include <userspace/lib.h>


SECTION(".user.text")
void user_init() {
    pid_t pid;
    pid = fork();
    int i;
    if (pid == 0) {
        i = 2;
    } else {
        i = 3;
    }

    exit(i);
}
