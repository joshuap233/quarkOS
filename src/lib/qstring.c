//
// Created by pjs on 2021/1/5.
//
#include <stddef.h>
#include <stdbool.h>
#include "qstring.h"

size_t q_strlen(const char *str) {
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

char *q_strcat(char *dest, const char *src) {
    return q_strncat(dest, src, q_strlen(src));
}

// 加到第 n 个字符或遇到空字符停止
char *q_strncat(char *dest, const char *src, size_t n) {
    size_t id = q_strlen(dest);
    for (size_t is = 0; is < n; is++) {
        if (!src[is]) {
            break;
        }
        dest[id++] = src[is];
    }
    dest[id] = '\0';
    return dest;
}

bool q_strcmp(const char *s1, const char *s2) {
    size_t len = q_strlen(s1);
    for (size_t i = 0; i <= len; ++i) {
        if (s1[i] != s2[i])
            return false;
    }
    return true;
}

void *q_memcpy(void *dest, const void *src, size_t n) {
    char *cd = dest;
    const char *cs = src;

    for (size_t i = 0; i < n; ++i) {
        cd[i] = cs[i];
    }
    return cd;
}

void *q_memset(void *s, uint32_t c, size_t n) {
    unsigned char *_s = s;
    for (size_t i = 0; i < n; ++i) {
        _s[i] = c;
    }
    return s;
}

void q_bzero(void *s, size_t n) {
    q_memset(s, 0, n);
}