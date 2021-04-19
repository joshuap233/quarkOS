//
// Created by pjs on 2021/3/17.
//
// PATA

#ifndef QUARKOS_DRIVERS_IDE_H
#define QUARKOS_DRIVERS_IDE_H

#include "buf.h"
#include "x86.h"
#include "isr.h"
#include "lib/qlib.h"

#define IDE_IO_BASE     0x1F0
#define IDE_CMD        (IDE_IO_BASE + 7)  // W/写入指令
#define IDE_ERR        (IDE_IO_BASE + 1)  // R/Error Register

#define ATA_CMD_READ_DMA        0xC8
#define ATA_CMD_WRITE_DMA       0xCA

void ide_rw(buf_t *buf);
void ide_driver_init(buf_t *buf, uint32_t secs_cnt);
void ide_isr_handler(UNUSED interrupt_frame_t *frame);
INLINE void ide_send_cmd(uint8_t cmd){
    outb(IDE_CMD, cmd);
    assertk(inb(IDE_ERR)==0);
}

#endif //QUARKOS_DRIVERS_IDE_H
