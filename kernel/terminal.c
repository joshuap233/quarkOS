//
// Created by pjs on 2021/6/9.
//
#include <stdarg.h> // 可变参数
#include <types.h>
#include <terminal.h>
#include <drivers/keyboard.h>
#include <drivers/ps2.h>
#include <drivers/vga.h>
#include <isr.h>
#include <lib/qstring.h>
#include <lib/qlib.h>
#include <task/timer.h>
#include <drivers/lapic.h>

INT kb_isr(interrupt_frame_t *frame);

#define U64LEN 20           // uint64 十进制数长度
#define KB_BUFFER_SIZE  256 //键盘缓冲区大小

typedef struct kb_queue {
    uint8_t buffer[KB_BUFFER_SIZE];
    size_t header;
    size_t tail;
} kb_queue_t;

void terminal_init() {
    reg_isr(IRQ0+IRQ_KBD, kb_isr);
}

// 键盘输入
static kb_status_t kbStatus = {
        .e0       = false,
        .f0       = false,
        .lShift   = RELEASE,
        .rShift   = RELEASE,
        .lCtrl    = RELEASE,
        .rCtrl    = RELEASE,
        .lGui     = RELEASE,
        .lAlt     = RELEASE,
        .rAlt     = RELEASE,
        .numLk    = RELEASE,
        .capsLK   = RELEASE,
        .scrollLk = RELEASE
};

//TODO:单一缓冲区多终端会出现问题
kb_queue_t kb_buf = {
        .header=0,
        .tail = 0
};


void kb_clear(kb_queue_t *q) {
    q->tail = q->header;
}

void q_append(kb_queue_t *q, uint8_t c) {
    q->buffer[q->tail] = c;
    q->tail = (q->tail + 1) % KB_BUFFER_SIZE;
}

char q_pop(kb_queue_t *q) {
    assertk(q);

    if (q->header == q->tail) return KB_NULL;
    uint8_t res = q->buffer[q->header];
    q->header = (q->header + 1) % KB_BUFFER_SIZE;
    return (char) res;
}


//处理修饰键
static void kb_meta(uint8_t type, status_t s) {
    switch (type) {
        case LEFT_ALT:
            kbStatus.lAlt = s;
            break;
        case LEFT_CTRL:
            kbStatus.lCtrl = s;
            break;
        case LEFT_SHIFT:
            kbStatus.lShift = s;
            break;
        case RIGHT_ALT:
            kbStatus.rAlt = s;
            break;
        case RIGHT_CTRL:
            kbStatus.rCtrl = s;
            break;
        case RIGHT_SHIFT:
            kbStatus.rShift = s;
            break;
        case LEFT_GUI:
            kbStatus.lGui = s;
            break;
        default:
            break;
    }
}


//处理功能键
static void kb_fn(uint8_t scancode) {
    switch (scancode) {
        case NUMBER_LOCK:
            kbStatus.numLk = !kbStatus.numLk;
            break;
        case CAPSLOCK:
            kbStatus.capsLK = !kbStatus.capsLK;
            break;
        case SCROLL_LOCK:
            kbStatus.scrollLk = !kbStatus.scrollLk;
            break;
        case CURSOR_LEFT:
            vga_cursor_left();
            break;
        case CURSOR_UP:
            vga_cursor_up();
            break;
        case CURSOR_RIGHT:
            vga_cursor_right();
            break;
        case CURSOR_DOWN:
            vga_cursor_down();
            break;
        case BACKSPACE:
            vga_delete();
            break;
        default:
            break;
    }
}

static key_type_t kb_key_type(uint8_t value) {
    if (value < F1) {
        return CHAR;
    }
    if (value >= LEFT_ALT && value <= LEFT_GUI) {
        return META;
    }
    if (value <= PAGE_UP && value >= ESC) {
        return FN;
    }
    if (value >= M_WWW_SEARCH && value <= M_MEDIA_SELECT) {
        return MEDIA;
    }
    if (value <= F12 && value >= F1) {
        return F;
    }
    return ACPI;
}

static void kb_press_handle(uint8_t r) {
    key_type_t kp = kb_key_type(r);
    kbStatus.e0 = false;
    switch (kp) {
        case CHAR:
            q_append(&kb_buf, r);
            break;
        case META:
            kb_meta(r, PRESS);
            break;
        case FN:
            kb_fn(r);
            break;
        default:
            break;
    }
}

static void kb_release_handle(uint8_t r) {
    //断码,字符键与功能键断码不处理
    key_type_t kp = kb_key_type(r);
    kbStatus.e0 = false;
    kbStatus.f0 = false;
    switch (kp) {
        case META:
            kb_meta(r, RELEASE);
            break;
        default:
            break;
    }
}

//解析扫描码
static void kb_sc_parse(uint8_t scancode) {
    // 需要特别解析解析的键: pause 以及 print screen
    uint8_t r;
    if (scancode == 0xe0)
        kbStatus.e0 = true;
    else if (scancode == 0xf0)
        kbStatus.f0 = true;
    else {
        if (!kbStatus.f0) {
            //通码
            if (!kbStatus.e0)
                r = (kbStatus.lShift == PRESS || kbStatus.rShift == PRESS)
                    ? asciiShift1[scancode]
                    : asciiNonShift1[scancode];
            else
                r = ascii2[scancode];
            assertk(r != KNULL);
            kb_press_handle(r);
        } else {
            r = kbStatus.e0 ? ascii2[scancode] : asciiShift1[scancode];
            assertk(r != KNULL);
            kb_release_handle(r);
        }
    }
}

// PIC 1 号中断,键盘输入
INT kb_isr(UNUSED interrupt_frame_t *frame) {
    kb_sc_parse(ps2_rd());
    lapicEoi();
}

// VGA 输出
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


void put_string(const char *data) {
    size_t size = strlen(data);
    for (size_t i = 0; i < size; i++)
        vga_put_char(data[i]);
    vga_sync_cursor();
}

void put_strings(const char *data, size_t size) {
    for (size_t i = 0; i < size; i++)
        vga_put_char(data[i]);
    vga_sync_cursor();
}

void put_char(char c) {
    vga_put_char(c);
    vga_sync_cursor();
}

__attribute__ ((format (printf, 1, 2))) void printfk(char *__restrict str, ...) {
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
                    put_char((char) va_arg(ap, int));
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


char getchar() {
    char c;
    kb_clear(&kb_buf);
    while ((c = q_pop(&kb_buf)) == KB_NULL) {
        assertk(ms_sleep(10));
    }
    return c;
}

int32_t k_gets(char *buf, int32_t len) {
    int32_t i;
    for (i = 0; i < len; ++i) {
        buf[i] = getchar();
        put_char(buf[i]);
        if (buf[i] == NEWLINE)
            break;
    }
    return i;
}

void k_puts(char *buf, int32_t len) {
    if (len < 0)
        put_string(buf);
    else
        put_strings(buf, len);
}

void terminal_clear() {
    vga_clean();
}
