#include "vga.h"
#include "x86.h"
#include "qstdint.h"
#include <stddef.h>
#include "qstring.h"
#include "qlib.h"

//即将读写的行与列
static cursor_t cursor;
static uint8_t terminal_color;
static uint16_t *terminal_buffer;

// fg，bg 为前景色与背景色
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | (bg << 4);
}

// 拼接 ascii 字符与显示属性
static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}


static inline void vga_set_buf(char c, uint8_t color, cursor_t cur) {
    // 根据行列设置buffer
    const size_t index = BUF_INDEX(cur.col, cur.row);
    terminal_buffer[index] = vga_entry(c, color);
}

static inline void vga_clean_line(uint8_t row) {
    q_memset16(&terminal_buffer[BUF_INDEX(0, row)], vga_entry(' ', terminal_color), VGA_WIDTH);
}

static inline void vga_scroll_up() {
    q_memcpy(terminal_buffer, &terminal_buffer[BUF_INDEX(0, 1)], (VGA_HEIGHT - 1) * VGA_WIDTH * 2);
    vga_clean_line(VGA_HEIGHT - 1);
}


static inline void vga_clean() {
    // 清屏为空格
    q_memset16(terminal_buffer, vga_entry(' ', terminal_color), VGA_WIDTH * VGA_HEIGHT);
}


void vga_init() {
    cursor.row=0;
    cursor.col=0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_buffer = (uint16_t *) VGA_TEXT_MODE_MEM;
    vga_enable_cursor();
    vga_clean();
    vga_move_cursor(cursor);
}


void vga_set_color(uint8_t color) {
    terminal_color = color;
}

static void _vga_put_char(char c) {
    if (c == NEWLINE) {
        vga_newline();
        return;
    }

    vga_set_buf(c, terminal_color, cursor);
    if (++cursor.col == VGA_WIDTH) {
        vga_newline();
    }
}

void vga_newline() {
    if (cursor.row + 1 == VGA_HEIGHT)
        vga_scroll_up();
    else
        cursor.row++;
    cursor.col = 0;
}

void vga_enable_cursor() {
//    outb(VGA_INDEX, MAX_SLI);
//    outb(VGA_DAT, 0xf);     //设置字符高度(像素),最大为16-1

    //设置光标绘制在 0x0E-0x0F 像素
    outb(VGA_INDEX, S_CURSOR_Y);
    outb(VGA_DAT, 0x0E);

    outb(VGA_INDEX, E_CURSOR_Y);
    outb(VGA_DAT, 0x0F);
}

void vga_move_cursor(cursor_t cur) {
    uint16_t pos = BUF_INDEX(cur.col, cur.row);
    outb(VGA_INDEX, CI_L);
    outb(VGA_DAT, pos & BIT_MASK(uint8_t, 8));
    outb(VGA_INDEX, CI_H);
    outb(VGA_DAT, pos >> 8);
}

//长字符串输出结束调用,移动指针
void vga_move_end() {
    vga_move_cursor(cursor);
}

void vga_put_char(char c){
    _vga_put_char(c);
    vga_move_end();
}

void vga_put_string(const char *data) {
    size_t size = q_strlen(data);
    for (size_t i = 0; i < size; i++)
        _vga_put_char(data[i]);
    vga_move_end();
}
