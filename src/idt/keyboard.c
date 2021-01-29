//
// Created by pjs on 2021/1/28.
//
// 使用 scancode 2
#include "keyboard.h"
#include "stdint.h"
#include "ps2.h"
#include "qlib.h"
#include "qlib.h"

static device_status_t ds = {
        .error  = KB_ERROR,
        .resend = KB_RESEND,
        .ack    = KB_ACK
};


static kb_status_t kbStatus = {
        .e0       = false,
        .f0       = false,
        .lShift   = false,
        .rShift   = false,
        .lCtrl    = false,
        .rCtrl    = false,
        .lGui     = false,
        .lAlt     = false,
        .rAlt     = false,
        .numLk    = false,
        .capsLK   = false,
        .scrollLk = false
};


void ps2_kb_init() {
    //初始化 ps2 键盘
    //TODO: 使用队列管理指令

    uint8_t res;
    res = ps2_device_cmd(DisableScanning, ds);
    assertk(res != ds.error);

    //设置 scancode set 2
    res = ps2_device_cmd(0xF0, ds);
    assertk(res != ds.error);
    res = ps2_device_cmd(2, ds);
    assertk(res != ds.error);

    res = ps2_device_cmd(EnableScanning, ds);
    assertk(res != ds.error);
}

static void ps2_set_kb_status(uint8_t scancode) {
    switch (scancode) {
        case LEFT_ALT:
            kbStatus.lAlt = true;
            break;
        case LEFT_CTRL:
            kbStatus.lCtrl = true;
            break;
        case LEFT_SHIFT:
            kbStatus.lShift = true;
            break;
        case RIGHT_ALT:
            kbStatus.rAlt = true;
            break;
        case RIGHT_CTRL:
            kbStatus.rCtrl = true;
            break;
        case RIGHT_SHIFT:
            kbStatus.rShift = true;
            break;
        case LEFT_GUI:
            kbStatus.lGui = true;
            break;
        default:
            break;
    }
}

void ps2_sc_parse(uint8_t scancode) {
    // 需要特别解析解析的键: pause 以及 print screen
    uint8_t r;
    if (scancode == 0xe0) {
        kbStatus.e0 = true;
    } else if (scancode == 0xf0) {
        kbStatus.f0 = true;
    } else if (scancode < 0x83) {
        if (!kbStatus.f0 && !kbStatus.e0) {
            r = (kbStatus.lShift || kbStatus.rShift) ? asciiShift1[scancode] : asciiNonShift1[scancode];
            if (r == KNULL) {
                printfk("keyboard error: line 58\n");
                panic();
            }
            if (r < F1) {
                // ascii 字符
                printfk("%c", r);
            } else if (r >= F1 && r <= F12) {
                // F1-F12
            } else if (r >= LEFT_ALT && r <= LEFT_GUI) {
                //修饰键
                ps2_set_kb_status(r);
            }
        }

    } else {

    }
}