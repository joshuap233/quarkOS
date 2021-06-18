//
// Created by pjs on 2021/6/15.
//
// 直接嵌入内核, 方便 DEBUG

#include <types.h>
#include "lib.h"

#define TERMINAL_BUF_SIZE 256

char buf[TERMINAL_BUF_SIZE];


void hello() {
    char space[] = "                ";
    printf("\n");
    printf("%s************************************************\n", space);
    printf("%s*                                              *\n", space);
    printf("%s*                                              *\n", space);
    printf("%s*              Welcome to Quark OS             *\n", space);
    printf("%s*                                              *\n", space);
    printf("%s*                                              *\n", space);
    printf("%s************************************************\n", space);
    printf("\n");
}


int main() {
    cls();
    hello();
    while (1) {
        gets(buf, 10);
    }
    return 0;
}


