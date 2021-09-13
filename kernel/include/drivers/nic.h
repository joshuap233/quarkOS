//
// Created by pjs on 2021/6/18.
//

#ifndef QUARKOS_DRIVERS_NIC_H
#define QUARKOS_DRIVERS_NIC_H

#include <types.h>


// 接收描述符
struct e1000_rx_desc {
    volatile u64_t addr;     // 缓冲区物理地址
    volatile u16_t length;   // 硬件会设置包长度
    volatile u16_t checksum;
    volatile u8_t status;
    volatile u8_t errors;
    volatile u16_t special;
} PACKED;


// 发送描述符
struct e1000_tx_desc {
    volatile u64_t addr;
    volatile u16_t length;
    volatile u8_t cso;       // Checksum Offset
    volatile u8_t cmd;       // Command field
    volatile u8_t status;
    volatile u8_t css;       // Checksum Start Field
    volatile u16_t special;
} PACKED;

#endif //QUARKOS_DRIVERS_NIC_H
