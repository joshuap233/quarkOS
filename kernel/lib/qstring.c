//
// Created by pjs on 2021/1/5.
//
#include <lib/qstring.h>
#include <mm/kmalloc.h>
#include <types.h>
#include <lib/qlib.h>

size_t strlen(const char *str) {
    assertk(str);

    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

char *strcat(char *dest, const char *src) {
    assertk(dest && src);

    return strncat(dest, src, strlen(src));
}

// 加到第 n 个字符或遇到空字符停止
char *strncat(char *dest, const char *src, size_t n) {
    assertk(dest && src);


    size_t id = strlen(dest);
    for (size_t is = 0; is < n; is++) {
        if (!src[is]) {
            break;
        }
        dest[id++] = src[is];
    }
    dest[id] = '\0';
    return dest;
}

bool strcmp(const char *s1, const char *s2) {
    assertk(s1 && s2);

    size_t len = strlen(s1);
    return memcmp(s1, s2, len + 1);
}

bool memcmp(const void *s1, const void *s2, size_t len) {
    assertk(s1 && s2);

    const char *_s1 = s1, *_s2 = s2;
    for (size_t i = 0; i < len; ++i) {
        if (_s1[i] != _s2[i])
            return false;
    }
    return true;
}

void *memcpy(void *dest, const void *src, size_t n) {
    char *cd = dest;
    const char *cs = src;

    for (size_t i = 0; i < n; ++i) {
        cd[i] = cs[i];
    }
    return cd;
}

void *memset(void *s, uint32_t c, size_t n) {
    assertk(s);

    unsigned char *_s = s;
    for (size_t i = 0; i < n; ++i) {
        _s[i] = c;
    }
    return s;
}

// 以字为单位复制,n为需要复制的字的数量
void *memset16(void *s, uint16_t c, size_t n) {
    assertk(s);

    uint16_t *_s = s;
    for (size_t i = 0; i < n; ++i) {
        _s[i] = c;
    }
    return s;
}

void bzero(void *s, size_t n) {
    memset(s, 0, n);
}

//li 为最后一个索引值(非 '\0')
void str_reverse(char *const s, uint32_t li) {
    assertk(s);

    char temp;
    for (uint32_t i = 0, j = li; i < j; i++, j--) {
        temp = s[i];
        s[i] = s[j];
        s[j] = temp;
    }
}

void utoa(uint64_t value, char *str) {
    assertk(str);

    uint8_t i = 0;
    do {
        str[i++] = value % 10 + '0';
        value /= 10;
    } while (value != 0);
    str[i] = '\0';
    str_reverse(str, i - 1);
}


// 10 进制转 16 进制
void hex(uint64_t n, char *str) {
    assertk(str);

    static const char base[] = "0123456789abcdef";
    uint8_t rem, i = 0;

    do {
        rem = n % 16;
        n /= 16;
        str[i++] = base[rem];
    } while (n != 0);

    str[i] = '\0';
    str_reverse(str, i - 1);
}

char *strdup(const char *string) {
    assertk(string);

    u32_t len = strlen(string) + 1;
    char *new = kmalloc(len);
    memcpy(new, string, len + 1);
    return new;
}

