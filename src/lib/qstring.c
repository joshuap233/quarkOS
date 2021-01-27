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
    //TODO:使用 movsb 指令传输
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

// 以字为单位复制,n为需要复制的字的数量
void *q_memset16(void *s, uint16_t c, size_t n) {
    uint16_t *_s = s;
    for (size_t i = 0; i < n; ++i) {
        _s[i] = c;
    }
    return s;
}

void q_bzero(void *s, size_t n) {
    q_memset(s, 0, n);
}

//li 为最后一个索引值(非 '\0')
void reverse(char *const s, uint32_t li) {
    char temp;
    for (uint32_t i = 0, j = li; i < j; i++, j--) {
        temp = s[i];
        s[i] = s[j];
        s[j] = temp;
    }
}

void q_utoa(uint64_t value, char *str) {
    uint8_t i = 0;
    do {
        str[i++] = value % 10 + '0';
        value /= 10;
    } while (value != 0);
    str[i] = '\0';
    reverse(str, i - 1);
}


// 10 进制转 16 进制
void hex(uint64_t n, char *str) {
    static const char base[] = "0123456789abcdef";
    uint8_t rem, i = 0;

    do {
        rem = n % 16;
        n /= 16;
        str[i++] = base[rem];
    } while (n != 0);

    str[i] = '\0';
    reverse(str, i - 1);
}