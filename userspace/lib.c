//
// Created by pjs on 2021/6/15.
//

#include <types.h>
#include <stdarg.h> // 可变参数
#include "lib.h"

#define assert(condition) do{\
    if (!(condition)) {     \
        printf("\nassert error: %s: %s: %u\n",__FILE__,__FUNCTION__,__LINE__); \
        exit(1);                        \
    }\
}while(0)

#define U64LEN 20           // uint64 十进制数长度

static void reverse(char *const s, uint32_t li) {
    assert(s);

    char temp;
    for (uint32_t i = 0, j = li; i < j; i++, j--) {
        temp = s[i];
        s[i] = s[j];
        s[j] = temp;
    }
}

static void hex(uint64_t n, char *str) {
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

static void utoa(uint64_t value, char *str) {
    uint8_t i = 0;
    do {
        str[i++] = value % 10 + '0';
        value /= 10;
    } while (value != 0);
    str[i] = '\0';
    reverse(str, i - 1);
}

static size_t strlen(const char *str) {
    assert(str);

    size_t len = 0;
    while (str[len])
        len++;
    return len;
}


static void put_string(const char *data) {
    puts(data, -1);
}


static void put_char(char c) {
    puts(&c, 1);
}

static void print_d(int64_t num) {
    uint8_t i = 0;
    char str[U64LEN + 2]; //1 位存符号,1 位存 '\0'
    if (num < 0) {
        str[i++] = '-';
        num = -num;
    }
    utoa(num, str + i);
    put_string(str);
}

static void print_u(uint64_t num) {
    char str[U64LEN + 1];
    utoa(num, str);
    put_string(str);
}

static void print_pointer(void *p) {
    char str[U64LEN + 3] = "0x";
    hex((ptr_t) p, str + 2);
    put_string(str);
}

static void print_hex(uint64_t x) {
    char str[U64LEN + 3] = "0x";
    hex(x, str + 2);
    put_string(str);
}


__attribute__ ((format (printf, 1, 2))) void printf(char *__restrict str, ...) {
    size_t str_len = strlen(str);
    va_list ap;
    va_start(ap, str);
    for (size_t i = 0; i < str_len; i++) {
        if (str[i] == '%' && (i + 1) < str_len) {
            switch (str[++i]) {
                case 'd':
                    print_d(va_arg(ap, int32_t));
                    break;
                case 'u':
                    print_u(va_arg(ap, u32_t));
                    break;
                case 's':
                    put_string(va_arg(ap, char*));
                    break;
                case 'c':
                    put_char(va_arg(ap, int));
                    break;
                case 'p':
                    print_pointer(va_arg(ap, void*));
                    break;
                case 'x':
                    print_hex(va_arg(ap, u32_t));
                    break;
                case 'l':
                    switch (str[++i]) {
                        case 'd':
                            print_d(va_arg(ap, int64_t));
                            break;
                        case 'u':
                            print_u(va_arg(ap, u64_t));
                            break;
                        case 'x':
                            print_hex(va_arg(ap, u64_t));
                            break;
                        default:
                            i--;
                    }
                    break;
                default:
                    i--;
                    put_char(str[i]);
            }
        } else {
            put_char(str[i]);
        }
    }
    va_end(ap);
}

bool memcmp(const void *s1, const void *s2, size_t len) {
    assert(s1 && s2);

    const char *_s1 = s1, *_s2 = s2;
    for (size_t i = 0; i < len; ++i) {
        if (_s1[i] != _s2[i])
            return false;
    }
    return true;
}

bool strcmp(const char *s1, const char *s2) {
    assert(s1 && s2);
    size_t len = strlen(s1);
    return memcmp(s1, s2, len + 1);
}