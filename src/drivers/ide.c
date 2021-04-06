//
// Created by pjs on 2021/3/17.
//
// ata pio
// 驱动只处理一根线连接一个设备的情况
// 使用 lba28 编址,因此磁盘大小 <= 128G

#include "drivers/ide.h"
#include "types.h"
#include "x86.h"
#include "klib/qlib.h"
#include "isr.h"

//primary bus port
#define IDE_IO_BASE     0x1F0
#define IDE_CTR_BASE    0x3F6


// LBA28 规范中, P_DAT 为 16 比特,其余为 8 比特
#define IDE_DAT        (IDE_IO_BASE + 0)  // Read/Write PIO data bytes
#define IDE_ERR        (IDE_IO_BASE + 1)  // Error Register
#define IDE_FEAT       (IDE_IO_BASE + 2)  // Features Register
#define IDE_COUNT      (IDE_IO_BASE + 3)  // Number of sectors to read/write
#define IDE_NUM        (IDE_IO_BASE + 4)  // Sector Number Register (LBAlo)
#define IDE_CL         (IDE_IO_BASE + 5)  // Partial Disk Sector address.
#define IDE_DH         (IDE_IO_BASE + 6)  // Used to select a drive and/or head.
#define IDE_STAT       (IDE_IO_BASE + 7)  // 读取状态
#define IDE_CMD        (IDE_IO_BASE + 7)  // 写入指令

#define IDE_CTR_STAT   (IDE_CTR_BASE + 0) // 读
#define IDE_CTR        (IDE_CTR_BASE + 0) // 写
#define IDE_CTR_DA     (IDE_CTR_BASE + 1) // Provides drive select and head select information.

//struct buf {
//    int flags;
//    uint dev;
//    uint blockno;
//    struct sleeplock lock;
//    uint refcnt;
//    struct buf *prev; // LRU cache list
//    struct buf *next;
//    struct buf *qnext; // disk queue
//    uchar data[BSIZE];
//};
//#define B_VALID 0x2  // buffer has been read from disk
//#define B_DIRTY 0x4  // buffer needs to be written to disk


struct error {
    uint8_t addr_mark_nf: 1;
    uint8_t track_zero_nf: 1;
    uint8_t aborted_cmd: 1;
    uint8_t media_change_req: 1;
    uint8_t id_nf: 1;
    uint8_t media_changed: 1;
    uint8_t uncorrectable_data: 1;
    uint8_t bad_block: 1;
};

#define IDE_STAT_ERR     1
#define IDE_STAT_DRQ     1<<3 //Set when the drive has PIO data to transfer, or is ready to accept PIO data.
#define IDE_STAT_SRV     1<<4
#define IDE_STAT_DF      1<<5
#define IDE_STAT_RDY     1<<6 //Bit is clear when drive is spun down, or after an error.
#define IDE_STAT_BSY     1<<7 //Indicates the drive is preparing to send/receive data (wait for it to clear)

static int32_t ide_wait(bool check_error);

static struct ide_device {
#define Sector_SIZE 512
#define MAX_N_SECS (256 * M)         // lba28 总扇区数
    bool dma;                        // 是否支持 dma
    bool lba48;                      // 是否支持 lba48
    uint32_t size;                   // 扇区数量
} ide_device;


typedef struct buf {
    bool dirty;
    uint8_t data[Sector_SIZE];
} buf_t;

void ide_isr(interrupt_frame_t *frame);

void ide_init() {
    // 使用 identify 指令检测...
    ide_wait(0);

    // 选择要操作的设备为主盘(primary bus, master drivers), 使用 LBA 寻址
    outb(IDE_DH, 0xE0);
    ide_wait(0);

    //发送 IDENTIFY 指令
    outb(IDE_CMD, 0xEC);
    ide_wait(0);

    // 检查设备是否存在
    assertk(inb(IDE_STAT) != 0 && ide_wait(1) == 0);

    // 读取IDE设备信息
    uint16_t buffer[256];
    insl(IDE_DAT, buffer, sizeof(buffer) / 4);

    ide_device.lba48 = buffer[83] & (1 << 10);
    //  88/93  dma 检测
//    ide_device.size = buffer[60] + ((uint32_t) buffer[61] << 16);

    reg_isr(46, ide_isr);
}

// 等待主盘可用
static int32_t ide_wait(bool check_error) {
    uint8_t r;
    while ((r = inb(IDE_STAT)) & IDE_STAT_BSY);

    if (check_error && (r & (IDE_STAT_DF | IDE_STAT_ERR)) != 0) {
        return -1;
    }

    return 0;
}

// PIC 14 号中断
// ata primary bus
INT ide_isr(UNUSED interrupt_frame_t *frame) {
    pic_eoi(46);
}
