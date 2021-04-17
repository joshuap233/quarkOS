//
// Created by pjs on 2021/4/14.
//
// ide dma
#include "drivers/pci.h"
#include "lib/qlib.h"
#include "isr.h"
#include "buf.h"
#include "drivers/dma.h"
#include "lib/list.h"
#include "sched/kthread.h"
#include "drivers/ide.h"


#define N_PRD   N_BUF
#define LH list_head_t

// PCI 控制器 IO 端口偏移(primary bus):
//  bus master register
#define BM_CMD          0x0
#define BM_STAT         0x2
#define BM_PRDT_ADDR    0x4  //0x4 - 0x7


// bm  的命令端口
#define BM_CMD_WRITE  0
#define BM_CMD_READ   (1<<3)
#define BM_CMD_START  1
#define BM_CMD_STOP   0


// bm 的状态端口
#define BM_STAT_IRQ        (1<<2)
#define BM_STAT_ERROR      (1<<1)
#define BM_STAT_BSY         1

//Physical Region Descriptor
struct prd {
    uint32_t addr; // 需要传输的数据缓冲区物理地址
    uint16_t size;  // 需要传输的字节数,
    uint16_t zero: 15;
    uint16_t end: 1; //最后一个 prd 设置该位
};

// prd table
_Alignas(4) struct prd prdt[N_PRD];
QUEUE_HEAD(dma_queue);

struct ide_dma_dev dma_dev;

INT dma_isr(UNUSED interrupt_frame_t *frame);

static void prdt_init();

void udma_init() {
    // ide 的 class 与 subclass 都为 1
    pci_dev_t *pci_dev = &dma_dev.pci_dev;
    pci_dev->class_code = 1;
    pci_dev->subclass = 1;

    // 懒得处理其他情况了...就这样吧
    // 事实上 prg_if 可能会有 8 钟
    if (pci_device_detect(pci_dev) != 0 || pci_dev->hd_type != 0 || pci_dev->prg_if != 128) {
        dma_dev.dma = false;
        return;
    }

    // hd_type=0 时, 后两位无效
    dma_dev.iob = pci_inl(pci_dev, PCI_BA_OFFSET0) & (MASK_U32(30) << 2);
    dma_dev.ctrl = pci_inl(pci_dev, PCI_BA_OFFSET1) & (MASK_U32(30) << 2);
    dma_dev.bm = pci_inl(pci_dev, PCI_BA_OFFSET4) & (MASK_U32(30) << 2);

    reg_isr(46, dma_isr);

    prdt_init();
    // prdt 地址写入 bus master 寄存器
    outl(dma_dev.bm + BM_PRDT_ADDR, (pointer_t) prdt);
}

static void prdt_init() {
    int i;
    for (i = 0; i < N_PRD; ++i) {
        prdt[i].size = SECTOR_SIZE;
    }
    prdt[i].end = 1;
}


void dma_start(buf_t *buf) {
    // TODO: 将需要读/写的磁盘地址按照 no_secs 排序 ?
    uint32_t i = 0;

    bool dirty = buf->flag & BUF_DIRTY;
    for (LH *hdr = &buf->queue; hdr != &dma_queue; hdr = hdr->next) {
        if ((buf_entry(hdr)->flag & BUF_DIRTY) == dirty) {
            prdt[i++].addr = (pointer_t) buf->data;
        }
    }
    prdt[i - 1].end = 1;

    uint8_t bm_cmd = dirty ? BM_CMD_WRITE : BM_CMD_READ;
    outb(dma_dev.bm + BM_CMD, bm_cmd);

    uint8_t stat = inb(dma_dev.bm + BM_STAT);
    outb(dma_dev.bm + BM_STAT, stat & (~(BM_STAT_IRQ | BM_STAT_ERROR)));
    ide_driver_init(buf, i);
    ide_send_cmd(dirty ? ATA_CMD_WRITE_DMA : ATA_CMD_READ_DMA);

    outb(dma_dev.bm + BM_CMD, bm_cmd | BM_CMD_START);
}

void dma_rw(buf_t *buf) {
    queue_put(&buf->queue, &dma_queue);
    if (&buf->queue == queue_head(&dma_queue)) {
        dma_start(buf);
    }
}


INT dma_isr(UNUSED interrupt_frame_t *frame) {
    uint8_t stat = inb(dma_dev.bm + BM_STAT);
    assertk(!((stat & (BM_STAT_ERROR | BM_STAT_BSY))));

    bool dirty = buf_entry(queue_head(&dma_queue))->flag & BUF_DIRTY;
    list_for_each(&dma_queue) {
        buf_t *buf = buf_entry(hdr);
        if ((buf_entry(hdr)->flag & BUF_DIRTY) == dirty) {
            unblock_thread(hdr);
            buf->flag |= BUF_VALID;
            buf->flag &= ~(BUF_DIRTY | BUF_BSY);
            list_del(hdr);
        }
    }

    if (!queue_empty(&dma_queue)) {
        dma_start(buf_entry(queue_head(&dma_queue)));
    }
    pic2_eoi(46);
}