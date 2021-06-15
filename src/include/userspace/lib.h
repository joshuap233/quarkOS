//
// Created by pjs on 2021/6/7.


#ifndef QUARKOS_USERSPACE_LIB_H
#define QUARKOS_USERSPACE_LIB_H

#include <types.h>

int fork();

int exit(int errno);

int puts(char *buf, int32_t len);

int gets(char *buf, int32_t len);

int exec(const char *path);

#endif //QUARKOS_USERSPACE_LIB_H
