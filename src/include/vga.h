#ifndef QUARKOS_VGA_H
#define QUARKOS_VGA_H

#include "qstdint.h"

#define VGA_TEXT_MODE_MEM 0xB8000

// VGA模式 3 提供 80 * 25的字符窗口显示
// 0xB8000 ~ 0xBFFFF 内存地址映射到显存(VGA text mode)
#define VGA_WIDTH    80
#define VGA_HEIGHT   25
#define BUF_INDEX(col, row) (row*VGA_WIDTH+col)
#define NEWLINE      '\n'
#define VGA_INDEX   0x3d4  //索引寄存器端口
#define VGA_DAT     0x3d5  //vga 数据端口
#define CI_H        0x0e   //光标位置索引:高 8 位
#define CI_L        0x0f   //光标位置索引:低 8 位索引
#define MAX_SLI     0x09   //光标最大 scan line 索引
#define S_CURSOR_Y  0x0A   //光标开始像素
#define E_CURSOR_Y  0x0B   //光标结束像素

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

//光标位置,即 即将读写的行列
typedef struct cursor {
    uint8_t row;
    uint8_t col;
}cursor_t;

void vga_init();

void vga_set_color(uint8_t color);

void vga_put_char(char c);

void vga_newline();

void vga_enable_cursor();

void vga_move_cursor(cursor_t cur);
void vga_move_end();

void vga_put_string(const char *data);

#endif //QUARKOS_VGA_H
