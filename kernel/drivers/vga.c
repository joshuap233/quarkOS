#include <types.h>
#include <drivers/vga.h>
#include <x86.h>
#include <lib/qstring.h>
#include <lib/qlib.h>
#include <highmem.h>


#define VGA_TEXT_MODE_MEM (0xB8000 + KERNEL_START)

// VGA模式 3 提供 80 * 25的字符窗口显示
// 0xB8000 ~ 0xBFFFF 内存地址映射到显存(VGA text mode)
#define VGA_WIDTH    80
#define VGA_HEIGHT   25
#define BUF_INDEX(col, row) ((row)*VGA_WIDTH+(col))
#define VGA_INDEX   0x3d4  //索引寄存器端口
#define VGA_DAT     0x3d5  //vga 数据端口
#define CI_H        0x0e   //光标位置索引:高 8 位
#define CI_L        0x0f   //光标位置索引:低 8 位索引
#define MAX_SLI     0x09   //光标最大 scan line 索引
#define S_CURSOR_Y  0x0A   //光标开始像素
#define E_CURSOR_Y  0x0B   //光标结束像素


void vga_enable_cursor();


//即将读写的行与列,不要直接修改
static cursor_t cursor;
static uint8_t terminal_color;
static uint16_t *terminal_buffer;
#define VGA_SPACE vga_entry(' ', terminal_color)


// fg，bg 为前景色与背景色
INLINE uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | (bg << 4);
}

// 拼接 ascii 字符与显示属性
INLINE uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}


INLINE void vga_set_buf(char c, uint8_t color, cursor_t cur) {
    // 根据行列设置buffer
    const size_t index = BUF_INDEX(cur.col, cur.row);
    terminal_buffer[index] = vga_entry(c, color);
}

INLINE void vga_clean_line(uint8_t row) {
    memset16(&terminal_buffer[BUF_INDEX(0, row)], VGA_SPACE, VGA_WIDTH);
}

INLINE void vga_scroll_up() {
    memcpy(terminal_buffer, &terminal_buffer[BUF_INDEX(0, 1)], (VGA_HEIGHT - 1) * VGA_WIDTH * 2);
    vga_clean_line(VGA_HEIGHT - 1);
}


INLINE void set_cursor(uint8_t row, uint8_t col) {
    cursor.row = row;
    cursor.col = col;
}


//指针向上平移
INLINE void cursor_up() {
    cursor.row = cursor.row == 0 ? 0 : cursor.row - 1;
}

//指针向下平移
INLINE void cursor_down() {
    uint8_t nr = cursor.row + 1;
    cursor.row = nr >= VGA_HEIGHT ? cursor.row : nr;
}

// 移动到下一行行首
INLINE void inc_row() {
    cursor.row + 1 == VGA_HEIGHT ? vga_scroll_up() : cursor.row++;
    cursor.col = 0;
}

// 移动到上一行行尾
INLINE void dec_row() {
    if (cursor.row != 0) {
        cursor.row--;
        cursor.col = VGA_WIDTH - 1;
    }
}

// cursor.col + 1
INLINE void inc_col() {
    cursor.col + 1 == VGA_WIDTH ? inc_row() : cursor.col++;
}

// cursor.col - 1
INLINE void dec_col() {
    cursor.col == 0 ? dec_row() : cursor.col--;
}


void __vga_clean() {
    // 清屏为空格
    memset16(terminal_buffer, VGA_SPACE, VGA_WIDTH * VGA_HEIGHT);
}

void __vga_cleanc(cursor_t c) {
    //清除一个字符
    memset16(&terminal_buffer[BUF_INDEX(c.col, c.row)], VGA_SPACE, 1);
}


void vga_clean() {
    set_cursor(0, 0);
    __vga_clean();
    vga_sync_cursor();
}

void vga_init() {
    set_cursor(0, 0);
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_buffer = (uint16_t *) VGA_TEXT_MODE_MEM;
    __vga_clean();
    vga_enable_cursor();
    vga_sync_cursor();
}


void vga_set_color(uint8_t color) {
    terminal_color = color;
}

void vga_put_char(char c) {
    // newline 与 tab 交给 console 处理?
    if (c == NEWLINE) {
        inc_row();
        return;
    }

    vga_set_buf(c, terminal_color, cursor);
    inc_col();
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

void vga_sync_cursor() {
    //同步指针到 vga 设备
    uint16_t pos = BUF_INDEX(cursor.col, cursor.row);
    outb(VGA_INDEX, CI_L);
    outb(VGA_DAT, pos & BIT_MASK(uint8_t, 8));
    outb(VGA_INDEX, CI_H);
    outb(VGA_DAT, pos >> 8);
}


void vga_delete() {
    //前移指针并删除一个字符
    dec_col();
    __vga_cleanc(cursor);
    vga_sync_cursor();
}

//指针左移
void vga_cursor_left() {
    dec_col();
    vga_sync_cursor();
}

//指针右移
void vga_cursor_right() {
    inc_col();
    vga_sync_cursor();
}

//指针上移
void vga_cursor_up() {
    cursor_up();
    vga_sync_cursor();

}

//指针下移
void vga_cursor_down() {
    cursor_down();
    vga_sync_cursor();
}
