//
// Created by pjs on 2021/1/5.
//

#ifndef QUARKOS_QSTRING_H
#define QUARKOS_QSTRING_H

#include <stddef.h>
#include <stdbool.h>
#include "qstdint.h"

size_t q_strlen(const char *str);

char *q_strcat(char *dest, const char *src);

char *q_strncat(char *dest, const char *src, size_t n);

// 与 c标准库返回值不一样
bool q_strcmp(const char *s1, const char *s2);

void *q_memcpy(void *dest, const void *src, size_t n);

//TODO:

// q_strcpy,q_strncpy
//void q_memset(void *dest, uint8_t val, uint32_t len);
//void q_bzero(void *dest, uint32_t len);

#endif //QUARKOS_QSTRING_H
