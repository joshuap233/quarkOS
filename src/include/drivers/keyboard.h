//
// Created by pjs on 2021/1/28.
//


#ifndef QUARKOS_DRIVERS_KEYBOARD_H
#define QUARKOS_DRIVERS_KEYBOARD_H

#include "types.h"
//状态读取
#define KB_ACK     0xFA
#define KB_RESEND  0xFE
#define KB_ERROR   0xFF

//指令
#define DisableScanning 0xF5 //键盘停止接收扫描码,防止数据冲突
#define EnableScanning  0xF4


// scan code set 2
// K_ 开头的为小键盘的键
// M_ 开头的为多媒体键(multimedia)
// A_ 开头的为 ACPI 键,现代键盘一般没有
// 非字符扫描码,扫描码从 1 开始
// 非字符扫描码值 > 0x83 ,以识别非字符扫描码
// 小键盘键当成大键盘同等键处理且小键盘不受 shift 影响
//数组占位
#define    KNULL         0

//字符键
#define    TAB          '\t'
#define    ENTER        '\n'
#define    SPACE        ' '

//字符 ascii < 0x84

// f1-f12 都在 asciiNonShift1 和 asciiShift1
//f1-f12 0x84-0x8f
#define    F1           0x84
#define    F2           (F1 +1)
#define    F3           (F2+1)
#define    F4           (F3+1)
#define    F5           (F4+1)
#define    F6           (F5+1)
#define    F7           (F6+1)
#define    F8           (F7+1)
#define    F9           (F8+1)
#define    F10          (F9+1)
#define    F11          (F10+1)
#define    F12          (F11+1)

//功能键 0x90-0xa0
#define    ESC                  0x90
#define    CAPSLOCK             (ESC+1)
#define    BACKSPACE            (CAPSLOCK+1)
#define    NUMBER_LOCK          (BACKSPACE+1)
#define    SCROLL_LOCK          (NUMBER_LOCK+1)
#define    RIGHT_GUI            (SCROLL_LOCK+1)
#define    APPS                 (RIGHT_GUI+1)  //右control左边的键
#define    END                  (APPS+1)
#define    HOME                 (END+1)
#define    INSERT               (HOME+1)
#define    DELETE               (INSERT+1)
#define    CURSOR_LEFT          (DELETE+1) //方向键
#define    CURSOR_DOWN          (CURSOR_LEFT+1)
#define    CURSOR_RIGHT         (CURSOR_DOWN+1)
#define    CURSOR_UP            (CURSOR_RIGHT+1)
#define    PAGE_DOWN            (CURSOR_UP+1)
#define    PAGE_UP              (PAGE_DOWN+1)



//修饰键 可以组合 0xa1-0xa7
#define    LEFT_ALT         0xa1
#define    LEFT_CTRL        (LEFT_ALT+1)
#define    LEFT_SHIFT       (LEFT_CTRL+1)
#define    RIGHT_ALT        (LEFT_SHIFT+1)
#define    RIGHT_CTRL       (RIGHT_ALT+1)
#define    RIGHT_SHIFT      (RIGHT_CTRL+1)
#define    LEFT_GUI         (RIGHT_SHIFT+1) // 左 windows 键


//媒体键，我也不知道是什么
//0xa8 - 0xb9
#define    M_WWW_SEARCH           0xa8
#define    M_PREVIOUS_TRACK       (M_WWW_SEARCH+1)
#define    M_WWW_FAVORITES        (M_PREVIOUS_TRACK+1)
#define    M_WWW_REFRESH          (M_WWW_FAVORITES+1)
#define    M_VOLUME_DOWN          (M_WWW_REFRESH+1)
#define    M_MUTE                 (M_VOLUME_DOWN+1)
#define    M_WWW_STOP             (M_MUTE+1)
#define    M_CALCULATOR           (M_WWW_STOP+1)
#define    M_WWW_FORWARD          (M_CALCULATOR+1)
#define    M_VOLUME_UP            (M_WWW_FORWARD+1)
#define    M_PLAY_PAUSE           (M_VOLUME_UP+1)  //play/pause键
#define    M_WWW_BACK             (M_PLAY_PAUSE+1)
#define    M_WWW_HOME             (M_WWW_BACK+1)
#define    M_STOP                 (M_WWW_HOME+1)
#define    M_MY_COMPUTER          (M_STOP+1)
#define    M_EMAIL                (M_MY_COMPUTER+1)
#define    M_NEXT_TRACK           (M_EMAIL+1)
#define    M_MEDIA_SELECT         (M_NEXT_TRACK+1)


// ACPI 键
//0xba - 0xbc
#define   A_POWER                 0xba
#define   A_SLEEP                 (A_POWER+1)
#define   A_WAKE                  (A_SLEEP+1)


//索引为扫描码
static const unsigned char asciiNonShift1[] = {
        [0x00] = KNULL,
        [0x01] = F9,
        [0x02] = KNULL,
        [0x03] = F5,
        [0x04] = F3,
        [0x05] = F1,
        [0x06] = F2,
        [0x07] = F12,
        [0x08] = KNULL,
        [0x09] = F10,
        [0x0A] = F8,
        [0x0B] = F6,
        [0x0C] = F4,
        [0x0D] = TAB,
        [0x0E] = '`',
        [0x0F] = KNULL,
        [0x10] = KNULL,
        [0x11] = LEFT_ALT,
        [0x12] = LEFT_SHIFT,
        [0x13] = KNULL,
        [0x14] = LEFT_CTRL,
        [0x15] = 'q',
        [0x16] = '1',
        [0x17] = KNULL,
        [0x18] = KNULL,
        [0x19] = KNULL,
        [0x1A] = 'z',
        [0x1B] = 's',
        [0x1C] = 'a',
        [0x1D] = 'w',
        [0x1E] = '2',
        [0x1F] = KNULL,
        [0x20] = KNULL,
        [0x21] = 'c',
        [0x22] = 'x',
        [0x23] = 'd',
        [0x24] = 'e',
        [0x25] = '4',
        [0x26] = '3',
        [0x27] = KNULL,
        [0x28] = KNULL,
        [0x29] = SPACE,
        [0x2A] = 'v',
        [0x2B] = 'f',
        [0x2C] = 't',
        [0x2D] = 'r',
        [0x2E] = '5',
        [0x2F] = KNULL,
        [0x30] = KNULL,
        [0x31] = 'n',
        [0x32] = 'b',
        [0x33] = 'h',
        [0x34] = 'g',
        [0x35] = 'y',
        [0x36] = '6',
        [0x37] = KNULL,
        [0x38] = KNULL,
        [0x39] = KNULL,
        [0x3A] = 'm',
        [0x3B] = 'j',
        [0x3C] = 'u',
        [0x3D] = '7',
        [0x3E] = '8',
        [0x3F] = KNULL,
        [0x40] = KNULL,
        [0x41] = ',',
        [0x42] = 'k',
        [0x43] = 'i',
        [0x44] = 'o',
        [0x45] = '0',
        [0x46] = '9',
        [0x47] = KNULL,
        [0x48] = KNULL,
        [0x49] = '.',
        [0x4A] = '/',
        [0x4B] = 'l',
        [0x4C] = ';',
        [0x4D] = 'p',
        [0x4E] = '-',
        [0x4F] = KNULL,
        [0x50] = KNULL,
        [0x51] = KNULL,
        [0x52] = '\'',
        [0x53] = KNULL,
        [0x54] = '[',
        [0x55] = '=',
        [0x56] = KNULL,
        [0x57] = KNULL,
        [0x58] = CAPSLOCK,
        [0x59] = RIGHT_SHIFT,
        [0x5A] = ENTER,
        [0x5B] = ']',
        [0x5C] = KNULL,
        [0x5D] = '\\',
        [0x5E] = KNULL,
        [0x5F] = KNULL,
        [0x60] = KNULL,
        [0x61] = KNULL,
        [0x62] = KNULL,
        [0x63] = KNULL,
        [0x64] = KNULL,
        [0x65] = KNULL,
        [0x66] = BACKSPACE,
        [0x67] = KNULL,
        [0x68] = KNULL,
        [0x69] = '1',//keypad
        [0x6A] = KNULL,
        [0x6B] = '4',//keypad
        [0x6C] = '7',//keypad
        [0x6D] = KNULL,
        [0x6E] = KNULL,
        [0x6F] = KNULL,
        [0x70] = '0',//keypad
        [0x71] = '.',//keypad
        [0x72] = '2',//keypad
        [0x73] = '5',//keypad
        [0x74] = '6',//keypad
        [0x75] = '8',//keypad
        [0x76] = ESC,
        [0x77] = NUMBER_LOCK,
        [0x78] = F11,
        [0x79] = '+',//keypad
        [0x7A] = '3',//keypad
        [0x7B] = '-',//keypad
        [0x7C] = '*',//keypad
        [0x7D] = '9',//keypad
        [0x7E] = SCROLL_LOCK,
        [0x7F] = KNULL,
        [0x80] = KNULL,
        [0x81] = KNULL,
        [0x82] = KNULL,
        [0x83] = F7,
};


// asciiNonShift2 不受 shift 影响

// 0x83 后,每个键扫描码为2字节,第一字节为 0xe0
static const unsigned char ascii2[] = {
        KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL,
        [0x10] = M_WWW_SEARCH,
        [0x11] = RIGHT_ALT,
        [0x12] = KNULL, KNULL,
        [0x14] = RIGHT_CTRL,
        [0x15] = M_PREVIOUS_TRACK,
        [0x16] = KNULL, KNULL,
        [0x18] = M_WWW_FAVORITES,
        [0x19] = KNULL, KNULL, KNULL, KNULL, KNULL,
        [0x1F] = LEFT_GUI,
        [0x20] = M_WWW_REFRESH,
        [0x21] = M_VOLUME_DOWN,
        [0x22] = KNULL,
        [0x23] = M_MUTE,
        [0x24] = KNULL, KNULL, KNULL,
        [0x27] = RIGHT_GUI,
        [0x28] = M_WWW_STOP,
        [0x29] = KNULL, KNULL,
        [0x2B] = M_CALCULATOR,
        [0x2C] = KNULL, KNULL, KNULL,
        [0x2F] = APPS,
        [0x30] = M_WWW_FORWARD,
        [0x31] = KNULL,
        [0x32] = M_VOLUME_UP,
        [0x33] = KNULL,
        [0x34] = M_PLAY_PAUSE,
        [0x35] = KNULL, KNULL,
        [0x37] = A_POWER,
        [0x38] = M_WWW_BACK,
        [0x39] = KNULL,
        [0x3A] = M_WWW_HOME,
        [0x3B] = M_STOP,
        [0x3C] = KNULL, KNULL, KNULL,
        [0x3f] = A_SLEEP,
        [0x40] = M_MY_COMPUTER,
        [0x41] = KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL,
        [0x48] = M_EMAIL,
        [0x49] = KNULL,
        [0x4A] = '/',//keypad
        [0x4B] = KNULL, KNULL,
        [0x4D] = M_NEXT_TRACK,
        [0x4E] = KNULL, KNULL,
        [0x50] = M_MEDIA_SELECT,
        [0x51] = KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL,
        [0x5A] = ENTER, //keypad
        [0x5B] = KNULL, KNULL, KNULL,
        [0x5E] = A_WAKE,
        [0x5F] = KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL, KNULL,
        [0x69] = END,
        [0x6B] = CURSOR_LEFT,
        [0x6C] = HOME,
        [0x6D] = KNULL, KNULL, KNULL,
        [0x70] = INSERT,
        [0x71] = DELETE,
        [0x72] = CURSOR_DOWN,
        [0x73] = KNULL,
        [0x74] = CURSOR_RIGHT,
        [0x75] = CURSOR_UP,
        [0x76] = KNULL, KNULL, KNULL, KNULL,
        [0x7A] = PAGE_DOWN,
        [0x7B] = KNULL, KNULL,
        [0x7D] = KNULL
};

static const unsigned char asciiShift1[] = {
        [0x00] = KNULL,
        [0x01] = F9,
        [0x02] = KNULL,
        [0x03] = F5,
        [0x04] = F3,
        [0x05] = F1,
        [0x06] = F2,
        [0x07] = F12,
        [0x08] = KNULL,
        [0x09] = F10,
        [0x0A] = F8,
        [0x0B] = F6,
        [0x0C] = F4,
        [0x0D] = TAB,
        [0x0E] = '`',
        [0x0F] = KNULL,
        [0x10] = KNULL,
        [0x11] = LEFT_ALT,
        [0x12] = LEFT_SHIFT,
        [0x13] = KNULL,
        [0x14] = LEFT_CTRL,
        [0x15] = 'Q',
        [0x16] = '!',
        [0x17] = KNULL,
        [0x18] = KNULL,
        [0x19] = KNULL,
        [0x1A] = 'Z',
        [0x1B] = 'S',
        [0x1C] = 'A',
        [0x1D] = 'W',
        [0x1E] = '@',
        [0x1F] = KNULL,
        [0x20] = KNULL,
        [0x21] = 'C',
        [0x22] = 'X',
        [0x23] = 'D',
        [0x24] = 'E',
        [0x25] = '$',
        [0x26] = '#',
        [0x27] = KNULL,
        [0x28] = KNULL,
        [0x29] = SPACE,
        [0x2A] = 'V',
        [0x2B] = 'F',
        [0x2C] = 'T',
        [0x2D] = 'R',
        [0x2E] = '%',
        [0x2F] = KNULL,
        [0x30] = KNULL,
        [0x31] = 'N',
        [0x32] = 'B',
        [0x33] = 'H',
        [0x34] = 'G',
        [0x35] = 'Y',
        [0x36] = '^',
        [0x37] = KNULL,
        [0x38] = KNULL,
        [0x39] = KNULL,
        [0x3A] = 'M',
        [0x3B] = 'J',
        [0x3C] = 'U',
        [0x3D] = '&',
        [0x3E] = '*',
        [0x3F] = KNULL,
        [0x40] = KNULL,
        [0x41] = '<',
        [0x42] = 'K',
        [0x43] = 'I',
        [0x44] = 'O',
        [0x45] = ')',
        [0x46] = '(',
        [0x47] = KNULL,
        [0x48] = KNULL,
        [0x49] = '>',
        [0x4A] = '?',
        [0x4B] = 'L',
        [0x4C] = ':',
        [0x4D] = 'P',
        [0x4E] = '_',
        [0x4F] = KNULL,
        [0x50] = KNULL,
        [0x51] = KNULL,
        [0x52] = '"',
        [0x53] = KNULL,
        [0x54] = '{',
        [0x55] = '+',
        [0x56] = KNULL,
        [0x57] = KNULL,
        [0x58] = CAPSLOCK,
        [0x59] = RIGHT_SHIFT,
        [0x5A] = ENTER,
        [0x5B] = '}',
        [0x5C] = KNULL,
        [0x5D] = '|',
        [0x5E] = KNULL,
        [0x5F] = KNULL,
        [0x60] = KNULL,
        [0x61] = KNULL,
        [0x62] = KNULL,
        [0x63] = KNULL,
        [0x64] = KNULL,
        [0x65] = KNULL,
        [0x66] = BACKSPACE,
        [0x67] = KNULL,
        [0x68] = KNULL,
        [0x69] = '1',//keypad
        [0x6A] = KNULL,
        [0x6B] = '4',//keypad
        [0x6C] = '7',//keypad
        [0x6D] = KNULL,
        [0x6E] = KNULL,
        [0x6F] = KNULL,
        [0x70] = '0',//keypad
        [0x71] = '.',//keypad
        [0x72] = '2',//keypad
        [0x73] = '5',//keypad
        [0x74] = '6',//keypad
        [0x75] = '8',//keypad
        [0x76] = ESC,
        [0x77] = NUMBER_LOCK,
        [0x78] = F11,
        [0x79] = '+',//keypad
        [0x7A] = '3',//keypad
        [0x7B] = '-',//keypad
        [0x7C] = '*',//keypad
        [0x7D] = '9',//keypad
        [0x7E] = SCROLL_LOCK,
        [0x7F] = KNULL,
        [0x80] = KNULL,
        [0x81] = KNULL,
        [0x82] = KNULL,
        [0x83] = F7,
};

typedef bool status_t;
#define PRESS   true
#define RELEASE false

//键盘状态
typedef struct kb_status {
    bool e0; //是否收到 e0
    bool f0; //是否收到 f0
    status_t lShift;
    status_t rShift;
    status_t lCtrl;
    status_t rCtrl;
    status_t lGui;   // 左 win 键
    status_t lAlt;
    status_t rAlt;
    status_t numLk; //NUMBER_LOCK
    status_t capsLK; //CAPSLOCK
    status_t scrollLk; //SCROLL_LOCK
} kb_status_t;

typedef struct kb_queue {
#define KB_BUFFER_SIZE  256 //键盘缓冲区大小
#define KB_NULL         '\0'
    uint8_t buffer[KB_BUFFER_SIZE];
    size_t header;
    size_t tail;
} kb_queue_t;

//解析扫描码
void kb_sc_parse(uint8_t scancode);

char kb_getchar();

typedef enum KEY_TYPE {
    CHAR = 0,
    F = 1, //F1-F12
    FN = 2, //功能键
    META = 3, //修饰键
    MEDIA = 4, //媒体键
    ACPI = 5, //电源键
} key_type_t;


key_type_t kb_key_type(uint8_t value);

#endif //QUARKOS_DRIVERS_KEYBOARD_H
