#include <stdbool.h> // bool datatype
#include <stddef.h> // size_t and NULL
#include "qstdint.h" // ntx_t and uintx_t
#include "qstring.h"

#if defined(__linux__)
#warning "你没有使用跨平台编译器进行编译"
#endif

#if !defined(__i386__)
#warning "你没有使用 ix86-elf 编译器进行编译"
#endif

#if !defined(__STDC_HOSTED__)
#error "你没有使用 freestanding模式"
#endif


/* Hardware text mode color constants. */
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
};

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

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putchar(char c) {
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

void terminal_write(const char *data, size_t size) {
    for (size_t i = 0; i < size; i++)
        terminal_putchar(data[i]);
}

void terminal_writestring(const char *data) {
    terminal_write(data, strlen(data));
}

void kernel_main(void) {
    /* Initialize terminal interface */
    terminal_initialize();

    /* Newline support is left as an exercise. */
    terminal_writestring("Hello, kernel World!\n1");
}