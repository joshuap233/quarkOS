//
// Created by pjs on 2021/1/5.
//

#ifndef QUARKOS_LIB_QSTRING_H
#define QUARKOS_LIB_QSTRING_H

#include <types.h>

size_t strlen(const char *str);

char *strcat(char *dest, const char *src);

char *strncat(char *dest, const char *src, size_t n);

// 与 c标准库返回值不一样
bool strcmp(const char *s1, const char *s2);

void *memcpy(void *dest, const void *src, size_t n);

void *memset16(void *s, uint16_t c, size_t n);

void *memset(void *s, uint32_t c, size_t n);

void bzero(void *s, size_t n);
bool memcmp(const void *s1, const void *s2, size_t len);

// 10 进制转 16 进制,结果存在 str中
void hex(uint64_t n, char *str);


// 整数转字符,结果放在 str中
void utoa(uint64_t value, char *str);

//li 为最后一个索引(非 '\0')
// 字符串反转
void str_reverse(char *s, uint32_t li);

char *strdup(const char *string);

#endif //QUARKOS_LIB_QSTRING_H
