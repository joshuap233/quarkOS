//
// Created by pjs on 2021/1/27.
//

#ifndef QUARKOS_PS2_H
#define QUARKOS_PS2_H

#include "qstdint.h"

#define PS2_DAT    0x60 //读写数据端口
#define PS2_CMD    0x64 //读端口读状态,写端口写入指令

#define N_POLL     10  //轮询端口次数

void ps2_init();

//d -> Detect
void ps2_d_device();

void ps2_d_scode();

bool poll_status(uint8_t bit, status_t expect);

void ps2_wc(uint8_t value);

void ps2_wd(uint8_t value);

uint8_t ps2_rd();

#endif //QUARKOS_PS2_H
