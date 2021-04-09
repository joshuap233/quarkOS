//
// Created by pjs on 2021/1/28.
//
// 使用 scancode 2
#include "drivers/keyboard.h"
#include "types.h"
#include "drivers/ps2.h"
#include "lib/qlib.h"
#include "isr.h"

INT kb_isr(interrupt_frame_t *frame);

static device_status_t ds = {
        .error  = KB_ERROR,
        .resend = KB_RESEND,
        .ack    = KB_ACK
};


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

static kb_queue_t kb_cmd = {
        .header=0,
        .tail = 0
};

void q_append(kb_queue_t *q, uint8_t c) {
    q->buffer[q->tail] = c;
    //TODO:缓冲区覆盖问题
    q->tail = (q->tail + 1) % KB_BUFFER_SIZE;
}

char q_pop(kb_queue_t *q) {
    if (q->header == q->tail) return KB_NULL;
    char res = q->buffer[q->header];
    q->header = (q->header + 1) % KB_BUFFER_SIZE;
    return res;
}


static void kb_cmd_worker() {
    uint8_t c, res;
    while ((c = q_pop(&kb_cmd)) != KB_NULL) {
        res = ps2_device_cmd(c, ds);
        assertk(res != ds.error);
    }
}

void kb_init() {
    //初始化 ps2 键盘
    q_append(&kb_cmd, DisableScanning);
    //设置 scancode set 2
    q_append(&kb_cmd, 0xF0);
    q_append(&kb_cmd, 2);
    q_append(&kb_cmd, EnableScanning);
    kb_cmd_worker();

    reg_isr(33, kb_isr);
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
//TODO: 使用 map 或数组索引存储处理函数地址
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

key_type_t kb_key_type(uint8_t value) {
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

void kb_sc_parse(uint8_t scancode) {
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
    pic1_eoi();
}
