//
// Created by pjs on 2021/1/28.
//
// 使用 scancode 2
#include <drivers/keyboard.h>
#include <types.h>
#include <drivers/ps2.h>
#include <lib/qlib.h>

#define CMD_BUFFER_SIZE  256 // 指令缓冲区大小

struct cmd_queue {
    uint8_t buffer[CMD_BUFFER_SIZE];
    size_t header;
    size_t tail;
} kb_queue_t;

static device_status_t ds = {
        .error  = KB_ERROR,
        .resend = KB_RESEND,
        .ack    = KB_ACK
};

static struct cmd_queue kb_cmd = {
        .header=0,
        .tail = 0
};


static void queue_append(struct cmd_queue *q, uint8_t c) {
    q->buffer[q->tail] = c;
    q->tail = (q->tail + 1) % CMD_BUFFER_SIZE;
}

static char queue_pop(struct cmd_queue *q) {
    if (q->header == q->tail) return KB_NULL;
    char res = q->buffer[q->header];
    q->header = (q->header + 1) % CMD_BUFFER_SIZE;
    return res;
}


static void kb_cmd_worker() {
    uint8_t c, res;
    while ((c = queue_pop(&kb_cmd)) != KB_NULL) {
        res = ps2_device_cmd(c, ds);
        assertk(res != ds.error);
    }
}

void kb_init() {
    //初始化 ps2 键盘
    queue_append(&kb_cmd, DisableScanning);
    //设置 scancode set 2
    queue_append(&kb_cmd, 0xF0);
    queue_append(&kb_cmd, 2);
    queue_append(&kb_cmd, EnableScanning);
    kb_cmd_worker();
}
