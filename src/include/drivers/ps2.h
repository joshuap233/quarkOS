//
// Created by pjs on 2021/1/27.
//

#ifndef QUARKOS_PS2_H
#define QUARKOS_PS2_H

#include "types.h"
#include "x86.h"

#define PS2_DAT    0x60 //读写数据端口
#define PS2_CMD    0x64 //读端口读状态,写端口写入指令


//向键盘发送指令后,键盘响应码(状态)
typedef struct device_status {
    uint8_t ack;
    uint8_t resend;
    uint8_t error;
    //...
} device_status_t;

void ps2_init();

__attribute__((always_inline))
static inline uint8_t ps2_rd() {
    return inb(PS2_DAT);
}

// 向设备发送指令,并接收响应
uint8_t ps2_device_cmd(uint8_t cmd, device_status_t status);


#endif //QUARKOS_PS2_H
