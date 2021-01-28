//
// Created by pjs on 2021/1/28.
//


#ifndef QUARKOS_KEYBOARD_H
#define QUARKOS_KEYBOARD_H
//状态读取
#define ACK     0xFA
#define RESEND  0xFE
#define ERROR   0xFF

//scan code set 2
// 非字符扫描码,扫描码从 1 开始
// 设置 非字符扫描码 > xxx ,以识别非字符扫描码
#define    NULL         0
#define    F1         0x05
#define    F2         0x06
#define    F3         0x04
#define    F4         0x0C
#define    F5         0x03
#define    F6           0x0B
#define    F7          0x0B
#define    F8            0x0A
#define    F9         0x01
#define    F10          0x09
#define    F11         0x0B
#define    F12        0x07

#define   TAB    0x0D
#define    ESC    27
#define   L_ALT     0xa
#define   L_SHIFT   0xa
#define   L_CTRL   0xa
#define   R_ALT     0xa
#define   R_SHIFT   0xa
#define   R_CTRL   0xa
#define   SPACE   0xa
#define   CapsLock   0xa
#define   ENTER   0xa
#define   BACKSPACE   0xa
#define   NumberLock   0xa
#define   ScrollLock   0xa

static const unsigned char asciiNonShift[] = {
        [0x00] = NULL,
        [0x01] = F9,
        [0x02] = NULL,
        [0x03] = F5,
        [0x04] = F3,
        [0x06] = F2,
        [0x07] = F12,
        [0x08] = NULL,
        [0x09] = F10,
        [0x0A] = F8,
        [0x0B] = F6,
        [0x0C] = F4,
        [0x0D] = TAB,
        [0x0E] = '`',
        [0x0F] = NULL,
        [0x10] = NULL,
        [0x11] = L_ALT,
        [0x12] = L_SHIFT,
        [0x13] = NULL,
        [0x14] = L_CTRL,
        [0x15] = 'q',
        [0x16] = 'q',
        [0x17] = NULL,
        [0x18] = NULL,
        [0x19] = NULL,
        [0x1A] = 'z',
        [0x1B] = 's',
        [0x1C] = 'a',
        [0x1D] = 'w',
        [0x1E] = '2',
        [0x1F] = NULL,
        [0x20] = NULL,
        [0x21] = 'c',
        [0x22] = 'x',
        [0x23] = 'd',
        [0x24] = 'e',
        [0x25] = '4',
        [0x26] = '3',
        [0x27] = NULL,
        [0x28] = NULL,
        [0x29] = SPACE,
        [0x2A] = 'v',
        [0x2B] = 'f',
        [0x2C] = 't',
        [0x2D] = 'r',
        [0x2E] = '5',
        [0x2F] = NULL,
        [0x30] = NULL,
        [0x31] = 'n',
        [0x32] = 'b',
        [0x33] = 'h',
        [0x34] = 'g',
        [0x35] = 'y',
        [0x36] = '6',
        [0x37] = NULL,
        [0x38] = NULL,
        [0x39] = NULL,
        [0x3A] = 'm',
        [0x3B] = 'j',
        [0x3C] = 'u',
        [0x3D] = '7',
        [0x3E] = '8',
        [0x3F] = NULL,
        [0x40] = NULL,
        [0x41] = ',',
        [0x42] = 'k',
        [0x43] = 'i',
        [0x44] = 'o',
        [0x45] = '0',
        [0x46] = '9',
        [0x47] = NULL,
        [0x48] = NULL,
        [0x49] = '.',
        [0x4A] = '/',
        [0x4B] = 'l',
        [0x4C] = ';',
        [0x4D] = 'p',
        [0x4E] = '-',
        [0x4F] = NULL,
        [0x50] = NULL,
        [0x51] = NULL,
        [0x52] = '\'',
        [0x53] = NULL,
        [0x54] = '[',
        [0x55] = '=',
        [0x56] = NULL,
        [0x57] = NULL,
        [0x58] = CapsLock,
        [0x59] = R_SHIFT,
        [0x5A] = ENTER,
        [0x5B] = ']',
        [0x5C] = NULL,
        [0x5D] = '\\',
        [0x5E] = NULL,
        [0x5F] = NULL,
        [0x60] = NULL,
        [0x61] = NULL,
        [0x62] = NULL,
        [0x63] = NULL,
        [0x64] = NULL,
        [0x65] = NULL,
        [0x66] = BACKSPACE,
        [0x67] = NULL,
        [0x68] = NULL,
        [0x69] = '1',
        [0x6A] = NULL,
        [0x6B] = '4',
        [0x6C] = '7',
        [0x6D] = NULL,
        [0x6E] = NULL,
        [0x6F] = NULL,
        [0x70] = '0',
        [0x71] = '.',
        [0x72] = '2',
        [0x73] = '5',
        [0x74] = '6',
        [0x75] = '8',
        [0x76] = ESC,
        [0x77] = NumberLock,
        [0x78] = F11,
        [0x79] = '+',
        [0x7A] = '3',
        [0x7B] = '-',
        [0x7C] = '*',
        [0x7D] = '9',
        [0x7E] = ScrollLock,
        [0x7F] = NULL,
        [0x80] = NULL,
        [0x81] = NULL,
        [0x82] = NULL,
        [0x83] = F7,
};
void ps2_kb_init();

#endif //QUARKOS_KEYBOARD_H
