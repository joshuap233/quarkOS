//
// Created by pjs on 2021/6/15.
//
// 直接嵌入内核, 方便 DEBUG

#include <types.h>
#include <task/task.h>
#include <userspace/lib.h>

#define TERMINAL_BUF_SIZE 256

SECTION(".user.data")
char buf[TERMINAL_BUF_SIZE];


SECTION(".user.text")
void hello() {
    // 直接写成常量会被放到内核 rodata 区
    char space[] = "                ";
    char c1[] = "\n";
    char c2[] = "%s************************************************\n";
    char c3[] = "%s*                                              *\n";
    char c4[] = "%s*              Welcome to Quark OS             *\n";
    printf(c2, space);
    printf(c3, space);
    printf(c3, space);
    printf(c4, space);
    printf(c3, space);
    printf(c3, space);
    printf(c2, space);
    printf(c1);
}


SECTION(".user.text")
void user_init() {
    cls();
    hello();
    while (1) {
        gets(buf, 10);
    }
    exit(0);
}


