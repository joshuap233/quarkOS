//
// Created by pjs on 2021/3/17.
//
// pata pio/dma
// 驱动只处理一根线连接一个设备的情况
// 使用 lba28 编址,因此磁盘大小 <= 128G


#include "drivers/ide.h"
#include "types.h"
#include "x86.h"
#include "lib/qlib.h"
#include "isr.h"
#include "buf.h"
#include "sched/kthread.h"

//primary bus port
#define IDE_IO_BASE     0x1F0
#define IDE_CTR_BASE    0x3F6


// LBA28 规范中, P_DAT 为 16 比特,其余为 8 比特
#define IDE_DAT        (IDE_IO_BASE + 0)  // Read/Write PIO data bytes
#define IDE_ERR        (IDE_IO_BASE + 1)  // R/Error Register
#define IDE_FEAT       (IDE_IO_BASE + 1)  // W/Features Register
#define IDE_COUNT      (IDE_IO_BASE + 2)  // Number of sectors to read/write
#define IDE_NUM        (IDE_IO_BASE + 3)  // Sector Number Register (LBAlo)
#define IDE_CYL_L      (IDE_IO_BASE + 4)  // Partial Disk Sector address.
#define IDE_CYL_H      (IDE_IO_BASE + 5)  // Partial Disk Sector address.
#define IDE_DH         (IDE_IO_BASE + 6)  // Used to select a drive and/or head.
#define IDE_STAT       (IDE_IO_BASE + 7)
// R/读取状态,读取该端口后需要重新设置 IDE_CTR 以生成中断,读取 IDE_ALT_STAT不会影响中断的生成

#define IDE_CMD        (IDE_IO_BASE + 7)  // W/写入指令

#define IDE_ALT_STAT   (IDE_CTR_BASE + 0) // 读
#define IDE_CTR        (IDE_CTR_BASE + 0) // 写
#define IDE_CTR_DA     (IDE_CTR_BASE + 1) // Provides drive select and head select information.

// 发送读写指令后需要等待 BSY 位清空再传输数据
#define CMD_READ_SECS       0x20
#define CMD_WRITE_SECS      0x30
#define CMD_READ_MULTIPLE   0xC4
#define CMD_WRITE_MULTIPLE  0xC5
#define CMD_IDENTIFY        0xEC
#define CMD_FLUSH           0xE7

#define LBA_DRIVE0     0xE0  // Drive/Head 寄存器 4-7位,选择 lba模式,0主盘

//static struct ide_queue {
//    buf_t *head;
//    buf_t *tail;
//} ide_queue = {NULL, NULL};

QUEUE_HEAD(ide_queue);

static struct ide_device {
#define MAX_N_SECS (256 * M)         // lba28 总扇区数
    bool udma;                        // 是否支持 udma
    bool lba48;                      // 是否支持 lba48
    uint32_t size;                   // 扇区数量
} ide_device;

//Physical Region Descriptor
//static struct prd {
//    uint32_t addr; // 需要传输的数据缓冲区物理地址
//    uint16_t cnt;  // 需要传输的字节数
//    uint16_t zero: 15;
//    uint16_t end: 1; //最后一个 prd 设置该位
//} prd;
//
//// prd table
//static _Alignas(4) struct prd prdt[1];

//struct error {

//    uint8_t addr_mark_nf: 1;
//    uint8_t track_zero_nf: 1;
//    uint8_t aborted_cmd: 1;
//    uint8_t media_change_req: 1;
//    uint8_t id_nf: 1;
//    uint8_t media_changed: 1;
//    uint8_t uncorrectable_data: 1;
//    uint8_t bad_block: 1;
//};

#define IDE_STAT_ERR     1
#define IDE_STAT_DRQ     1<<3 //Set when the drive has PIO data to transfer, or is ready to accept PIO data.
#define IDE_STAT_SRV     1<<4
#define IDE_STAT_DF      1<<5
#define IDE_STAT_RDY     1<<6 //Bit is clear when drive is spun down, or after an error.
#define IDE_STAT_BSY     1<<7 //Indicates the drive is preparing to send/receive data (wait for it to clear)


static int32_t ide_wait(bool check_error);

static void ide_start(buf_t *buf);


INT ide_isr(interrupt_frame_t *frame);

INLINE void read_sector(void *buf) {
    insw(IDE_DAT, buf, SECTOR_SIZE / 2);
}

INLINE void write_sector(void *b) {
    for (uint16_t *buf = b; (void *) buf < b + SECTOR_SIZE / sizeof(uint16_t); buf++) {
        outw(IDE_DAT, *buf);
        // 刷新缓存
        outb(IDE_CMD, CMD_FLUSH);
    }
}


void ide_init() {
    ide_wait(0);

    // 选择要操作的设备为主盘(primary bus, master drivers),
    // 使用 LBA 寻址
    outb(IDE_DH, LBA_DRIVE0);
    //发送 IDENTIFY 指令
    outb(IDE_CMD, CMD_IDENTIFY);

    // 检查设备是否存在
    assertk(inb(IDE_ALT_STAT) != 0 && ide_wait(1) == 0);

    // 读取IDE设备信息
    uint16_t buffer[256];
    read_sector(buffer);

    ide_device.lba48 = buffer[83] & (1 << 10);
    //  88/93  dma 检测
    //  ide_device.size = buffer[60] + ((uint32_t) buffer[61] << 16);

    //开启磁盘中断
    outb(IDE_CTR, 0);

    reg_isr(46, ide_isr);
}

// 等待主盘可用
static int32_t ide_wait(bool check_error) {
    uint8_t r;
    while ((r = inb(IDE_ALT_STAT)) & IDE_STAT_BSY);

    if (check_error && (r & (IDE_STAT_DF | IDE_STAT_ERR)) != 0) {
        return -1;
    }
    return 0;
}

// PIC 14 号中断
// ata primary bus
INT ide_isr(UNUSED interrupt_frame_t *frame) {
    queue_t *h = queue_get(&ide_queue);
    if (h) {
        buf_t *buf = buf_entry(h);
        unblock_thread(h);
        if (!(buf->flag & BUF_DIRTY)) {
            read_sector(buf->data);
            buf->flag |= BUF_VALID;
        } else {
            buf->flag &= ~BUF_DIRTY;
        }
        if (!queue_empty(&ide_queue))
            ide_start(buf_entry(queue_head(&ide_queue)));
    }
    pic2_eoi(46);
}

static void ide_start(buf_t *buf) {
    ide_wait(0);

    // 需要读取的扇区数
    outb(IDE_COUNT, 1);
    // 写入 28 位 lba 值
    outb(IDE_NUM, buf->no_secs & 0xFF);
    outb(IDE_CYL_L, (buf->no_secs >> 8) & MASK_U8(8));
    outb(IDE_CYL_H, (buf->no_secs >> 16) & MASK_U8(8));
    // 选择 drive0
    outb(IDE_DH, LBA_DRIVE0 | ((buf->no_secs >> 24) & MASK_U8(4)));

    if (buf->flag & BUF_DIRTY) {
        outb(IDE_CMD, CMD_WRITE_SECS);
        write_sector(buf->data);
    } else {
        outb(IDE_CMD, CMD_READ_SECS);
    }
}


void ide_rw(buf_t *buf) {
    queue_put(&buf->queue, &ide_queue);
    if (&buf->queue == queue_head(&ide_queue)) {
        ide_start(buf);
    }
}



