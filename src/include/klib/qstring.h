//
// Created by pjs on 2021/1/5.
//

#ifndef QUARKOS_QSTRING_H
#define QUARKOS_QSTRING_H

#include <stdbool.h>
#include "types.h"

size_t q_strlen(const char *str);

char *q_strcat(char *dest, const char *src);

char *q_strncat(char *dest, const char *src, size_t n);

// 与 c标准库返回值不一样
bool q_strcmp(const char *s1, const char *s2);

void *q_memcpy(void *dest, const void *src, size_t n);

void *q_memset16(void *s, uint16_t c, size_t n);

void *q_memset(void *s, uint32_t c, size_t n);

void q_bzero(void *s, size_t n);


// 10 进制转 16 进制,结果存在 str中
void hex(uint64_t n, char *str);


// 整数转字符,结果放在 str中
void q_utoa(uint64_t value, char *str);

//li 为最后一个索引(非 '\0')
// 字符串反转
void reverse(char *s, uint32_t li);

//TODO:

// q_strcpy,q_strncpy

#endif //QUARKOS_QSTRING_H
