//
// Created by pjs on 2021/6/16.
//

#include <types.h>
#include <task/fork.h>

extern int fork();

extern int exit(int errno);

extern int exec(const char *path);

SECTION(".user.text")
void user_init() {
    int i;
    pid_t pid = fork();
    if (pid == 0) {
        exec("/bin/sh");
    } else if (pid > 0) {
        i = 0;
    }
    exit(0);
}

