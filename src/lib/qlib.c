#include <stdbool.h>
#include <stdarg.h> // 可变参数
#include <stddef.h> // size_t and NULL
#include "qstring.h"
#include "qlib.h"
#include "qstdint.h"
#include "vga.h"
// fg，bg 为前景色与背景色,屏幕上的每个字符对应着显存中的连续两个字节,
// 前一个是字符的 ASCII 码,后一个显示属性,
// 而显示属性的字节,高四位定义背景色,低四位定义前景色
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | (bg << 4);
}

// 拼接 ascii 字符与显示属性
static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

// VGA模式 3 提供 80 * 25的字符窗口显示
// 0xB8000 ~ 0xBFFFF 内存地址映射到显存,直接对该内存读写可以修改显存来显示字符
static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t *terminal_buffer;

void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_buffer = (uint16_t *) 0xB8000;
    // 清屏为空格
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}


void terminal_setcolor(uint8_t color) {
    terminal_color = color;
}

static void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

void reverse(char *s, uint32_t n) {
    char temp;
    for (uint32_t i = 0, j = n; i < j; i++, j--) {
        temp = s[i];
        s[i] = s[j];
        s[j] = temp;
    }
}

void q_itoa(uint64_t value, char *str){
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

void print_char(char c) {
    if (c != '\n') {
        terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
        if (++terminal_column == VGA_WIDTH) {
            terminal_column = 0;
            if (++terminal_row == VGA_HEIGHT)
                terminal_row = 0;
        }

    } else {
        if (++terminal_row == VGA_HEIGHT)
            terminal_row = 0;
        terminal_column = 0;
    }

}

void print_str(const char *data) {
    size_t size = q_strlen(data);
    for (size_t i = 0; i < size; i++)
        print_char(data[i]);
}


void print_u(uint64_t num) {
    char str[sizeof(uint64_t) + 1];
    q_itoa(num, str);
    print_str(str);
}

void print_pointer(void *p) {
    char str[sizeof(uint64_t) + 1 + 2] = "0x";
    hex((uint32_t) p, str + 2);
    print_str(str);
}

void print_hex(uint64_t x) {
    char str[sizeof(uint64_t) + 1 + 2] = "0x";
    hex(x, str + 2);
    print_str(str);
}


#ifdef __i386__

__attribute__ ((format (printf, 1, 2))) void printfk(char *__restrict str, ...) {
    size_t str_len = q_strlen(str);
    va_list ap;
    va_start(ap, str);
    for (size_t i = 0; i < str_len; i++) {
        if (str[i] == '%' && (i + 1) < str_len) {
            switch (str[++i]) {
                case 'u':
                    print_u(va_arg(ap, uint32_t));
                    break;
                case 's':
                    print_str(va_arg(ap, char*));
                    break;
                case 'c':
                    print_char((char) va_arg(ap, int));
                    break;
                case 'p':
                    print_pointer(va_arg(ap, void*));
                    break;
                case 'x':
                    print_hex(va_arg(ap, uint32_t));
                    break;
                case 'l':
                    switch (str[++i]) {
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
                    print_char(str[i]);
            }
        } else {
            print_char(str[i]);
        }
    }
    va_end(ap);
}

#endif