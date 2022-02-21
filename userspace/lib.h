//
// Created by pjs on 2021/6/7.


#ifndef QUARKOS_USERSPACE_LIB_H
#define QUARKOS_USERSPACE_LIB_H

#include <types.h>

int fork();

int exit(int errno);

int puts(const char *buf, int32_t len);

int gets(char *buf, int32_t len);

int exec(const char *path);
int cls();

void printf(char *__restrict str, ...);

bool memcmp(const void *s1, const void *s2, size_t len);

bool strcmp(const char *s1, const char *s2);

#endif //QUARKOS_USERSPACE_LIB_H
