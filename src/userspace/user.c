//
// Created by pjs on 2021/6/15.
//
// 直接嵌入内核,否则不方便 DEBUG

#include <types.h>
#include <task/task.h>
#include <userspace/lib.h>

SECTION(".user.text")
void user_init() {

    exit(0);
}


