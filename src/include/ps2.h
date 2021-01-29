//
// Created by pjs on 2021/1/27.
//

#ifndef QUARKOS_PS2_H
#define QUARKOS_PS2_H

#include "qstdint.h"
#include "x86.h"

#define PS2_DAT    0x60 //读写数据端口
#define PS2_CMD    0x64 //读端口读状态,写端口写入指令

#define N_POLL     10  //轮询端口次数

//向键盘发送指令后,键盘响应码(状态)
typedef struct device_status {
    uint8_t ack;
    uint8_t resend;
    uint8_t error;
    //...
} device_status_t;


void ps2_init();

bool poll_status(uint8_t bit, status_t expect);

void ps2_pwc(uint8_t value);

//poll write data
void ps2_pwd(uint8_t value);

uint8_t ps2_prd();


//读取状态寄存器
static inline uint8_t ps2_rs() {
    return inb(PS2_CMD);
}


static inline uint8_t ps2_rd() {
    return inb(PS2_DAT);
}

static inline void ps2_wd(uint8_t value) {
    outb(PS2_DAT, value);
}

static inline void ps2_wc(uint8_t value) {
    outb(PS2_CMD, value);
}

//检测状态位
static inline bool ps2_cs(uint8_t bit, status_t expect) {
    return ((ps2_rs() >> bit) & 0b1) == expect;
}

static inline bool ps2_full_output() {
    return ps2_cs(0, NZ);
}

// 向设备发送指令,并接收响应
uint8_t ps2_device_cmd(uint8_t cmd, device_status_t status);


#endif //QUARKOS_PS2_H
