#include <stdarg.h> // 可变参数
#include <stddef.h> // size_t and NULL
#include "qstring.h"
#include "qlib.h"
#include "types.h"
#include "vga.h"
#include "qmath.h"

static void print_d(int64_t num) {
    uint8_t i = 0;
    char str[sizeof(uint64_t) + 1 + 1];//1 位存符号
    if (num < 0) {
        str[i++] = '-';
        num &= (BIT_MASK(uint64_t, 1) << 63);
    }
    q_utoa(num, str + i);
    vga_put_string(str);
}

static void print_u(uint64_t num) {
    char str[sizeof(uint64_t) + 1];
    q_utoa(num, str);
    vga_put_string(str);
}

static void print_pointer(void *p) {
    char str[sizeof(uint64_t) + 1 + 2] = "0x";
    hex((pointer_t) p, str + 2);
    vga_put_string(str);
}

static void print_hex(uint64_t x) {
    char str[sizeof(uint64_t) + 1 + 2] = "0x";
    hex(x, str + 2);
    vga_put_string(str);
}

#ifdef __i386__

__attribute__ ((format (printf, 1, 2))) void printfk(char *__restrict str, ...) {
    size_t str_len = q_strlen(str);
    va_list ap;
    va_start(ap, str);
    for (size_t i = 0; i < str_len; i++) {
        if (str[i] == '%' && (i + 1) < str_len) {
            switch (str[++i]) {
                case 'd':
                    print_d(va_arg(ap, int32_t));
                    break;
                case 'u':
                    print_u(va_arg(ap, uint32_t));
                    break;
                case 's':
                    vga_put_string(va_arg(ap, char*));
                    break;
                case 'c':
                    vga_put_char((char) va_arg(ap, int));
                    break;
                case 'p':
                    print_pointer(va_arg(ap, void*));
                    break;
                case 'x':
                    print_hex(va_arg(ap, uint32_t));
                    break;
                case 'l':
                    switch (str[++i]) {
                        case 'd':
                            print_d(va_arg(ap, int64_t));
                            break;
                        case 'u':
                            print_u(va_arg(ap, uint64_t));
                            break;
                        case 'x':
                            print_hex(va_arg(ap, uint64_t));
                            break;
                        default:
                            i--;
                    }
                    break;
                default:
                    i--;
                    vga_put_char(str[i]);
            }
        } else {
            vga_put_char(str[i]);
        }
    }
    va_end(ap);
}

#endif