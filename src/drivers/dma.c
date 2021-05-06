//
// Created by pjs on 2021/4/14.
//
// ide dma
// TODO: 有个 BUG 找不出来,我都快要疯了,我再也不想写驱动了!!!

#include "drivers/pci.h"
#include "lib/qlib.h"
#include "buf.h"
#include "drivers/dma.h"
#include "lib/list.h"
#include "sched/kthread.h"
#include "drivers/ide.h"
#include "mm/vmm.h"

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
    uint32_t addr;   // 需要传输的数据缓冲区物理地址
    uint16_t size;   // 需要传输的字节数,
    uint16_t zero: 15;
    uint16_t end: 1; // 最后一个 prd 设置该位
};

// prd table
static _Alignas(_Alignof(uint32_t)) struct prd prdt[N_PRD];

// 需要读写的缓冲块队列
static QUEUE_HEAD(queue);
#define HEAD  queue_head(&queue)

struct ide_dma_dev dma_dev;

static void prdt_init();


void dma_init() {
    dma_dev.dma = true;
    // ide 的 class 与 subclass 都为 1
    pci_dev_t *pci = &dma_dev.pci_dev;
    pci->class_code = 1;
    pci->subclass = 1;

    // 懒得处理其他情况了...就这样吧
    // 事实上 prg_if 可能会有 8 种
    if (pci_device_detect(pci) != 0 || pci->hd_type != 0 || pci->prg_if != 128) {
        dma_dev.dma = false;
        return;
    }

    // hd_type=0 时, 后两位无效
    uint32_t mask = MASK_U32(30) << 2;
//    dma_dev.iob = pci_inl(pci, PCI_BA_OFFSET0) & mask;
//    dma_dev.ctrl = pci_inl(pci, PCI_BA_OFFSET1) & mask;
    dma_dev.bm = pci_inl(pci, PCI_BA_OFFSET4) & mask;

    prdt_init();
    // prdt 地址写入 bus master 寄存器
    // outl(dma_dev.bm + BM_PRDT_ADDR, (ptr_t) prdt);
}

static void prdt_init() {
    int i;
    for (i = 0; i < N_PRD; ++i) {
        prdt[i].size = SECTOR_SIZE;
    }
    prdt[i - 1].end = 1;
}

void dma_buf_sort(buf_t *buf) {
    // 将需要读/写的磁盘地址按照 no_secs 排序
    bool dirty = buf->flag & BUF_DIRTY;
    LH *head = &buf->queue, *last = &buf->queue;
    for (LH *cur = head->next, *nxt = cur->next; cur != &queue; cur = nxt, nxt = nxt->next) {
        if ((buf_entry(cur)->flag & BUF_DIRTY) == dirty) {
            LH *hdr = last;
            for (; hdr != head && buf_entry(hdr)->no_secs > buf_entry(cur)->no_secs; hdr = hdr->prev);
            list_del(cur);
            list_add_next(cur, hdr);
            if (hdr == last) last = cur;
        }
    }
}

// 返回需要读扇区数
// TODO: 有些情况不需要排序,比如文件系统需要立即强制写入数据块
uint32_t dma_set_prdt(buf_t *buf) {
    dma_buf_sort(buf);
    uint32_t i = 0;
    bool dirty = buf->flag & BUF_DIRTY;
    for (LH *hdr = &buf->queue; hdr != &queue; hdr = hdr->next) {
        if ((buf_entry(hdr)->flag & BUF_DIRTY) == dirty) {
            prdt[i].end = 0;
            prdt[i++].addr = (ptr_t) buf->data;
            buf->flag |= BUF_BSY;
        }
    }
    prdt[i - 1].end = 1;
    return i;
}


void dma_start(buf_t *buf) {
    uint32_t i = dma_set_prdt(buf);
    outl(dma_dev.bm + BM_PRDT_ADDR, (ptr_t) prdt);

    bool dirty = buf->flag & BUF_DIRTY;
    uint8_t bm_cmd = dirty ? BM_CMD_WRITE : BM_CMD_READ;
    outb(dma_dev.bm + BM_CMD, bm_cmd);

    uint8_t state = inb(dma_dev.bm + BM_STAT);
    outb(dma_dev.bm + BM_STAT, state & (~(BM_STAT_IRQ | BM_STAT_ERROR)));

    ide_driver_init(buf, i);
    ide_send_cmd(dirty ? ATA_CMD_WRITE_DMA : ATA_CMD_READ_DMA);
    outb(dma_dev.bm + BM_CMD, bm_cmd | BM_CMD_START);
}

void dma_rw(buf_t *buf) {
    queue_put(&buf->queue, &queue);
    if (&buf->queue == HEAD) {
        dma_start(buf);
    }
}


void dma_isr_handler(UNUSED interrupt_frame_t *frame) {
    outb(dma_dev.bm + BM_CMD, BM_CMD_STOP);
    assertk(!(inb(dma_dev.bm + BM_STAT) & (BM_STAT_ERROR | BM_STAT_BSY)));

    bool dirty = buf_entry(HEAD)->flag & BUF_DIRTY;

    list_head_t *hdr, *next;
    list_for_each_del(hdr, next, &queue) {
        buf_t *buf = buf_entry(hdr);
        if ((buf->flag & BUF_DIRTY) == dirty) {
            unblock_threads(&buf->sleep);
            buf->flag |= BUF_VALID;
            buf->flag &= ~(BUF_DIRTY | BUF_BSY);
            list_del(hdr);

            // 通知线程, 中断处理程序以及被执行
            spinlock_unlock(&buf->lock);
        }
    }

    if (!queue_empty(&queue)) {
        dma_start(buf_entry(HEAD));
    }
}