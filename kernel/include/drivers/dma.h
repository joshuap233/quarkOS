//
// Created by pjs on 2021/4/15.
//

#ifndef QUARKOS_DRIVERS_DMA_H
#define QUARKOS_DRIVERS_DMA_H

#include "types.h"
#include "x86.h"
#include "drivers/pci.h"
#include "isr.h"

void dma_rw(struct page*buf);

struct ide_dma_dev {
    bool dma;                     // 是否支持 dma
//    uint32_t iob;              // 端口基地址(native mode)
//    uint32_t ctrl;             // 控制端口基地址(native mode)
    uint32_t bm;                 // 总线控制寄存器端口
    pci_dev_t pci_dev;
};
extern struct ide_dma_dev dma_dev;

void dma_isr_handler(UNUSED interrupt_frame_t *frame);


#endif //QUARKOS_DRIVERS_DMA_H
