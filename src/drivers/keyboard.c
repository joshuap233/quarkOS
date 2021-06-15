//
// Created by pjs on 2021/1/28.
//
// 使用 scancode 2
#include <drivers/keyboard.h>
#include <types.h>
#include <drivers/ps2.h>
#include <lib/qlib.h>

static device_status_t ds = {
        .error  = KB_ERROR,
        .resend = KB_RESEND,
        .ack    = KB_ACK
};

static kb_queue_t kb_cmd = {
        .header=0,
        .tail = 0
};

static void kb_cmd_worker() {
    uint8_t c, res;
    while ((c = kb_pop(&kb_cmd)) != KB_NULL) {
        res = ps2_device_cmd(c, ds);
        assertk(res != ds.error);
    }
}

void kb_init() {
    //初始化 ps2 键盘
    kb_append(&kb_cmd, DisableScanning);
    //设置 scancode set 2
    kb_append(&kb_cmd, 0xF0);
    kb_append(&kb_cmd, 2);
    kb_append(&kb_cmd, EnableScanning);
    kb_cmd_worker();
}
