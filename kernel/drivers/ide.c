//
// Created by pjs on 2021/3/17.
//
// pata pio
// 驱动只处理一根线连接一个设备的情况
// 使用 lba28 编址, 磁盘大小 <= 128G

#include <drivers/ide.h>
#include <lib/list.h>


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
#define CMD_READ_PIO        0x20
#define CMD_WRITE_PIO       0x30
#define CMD_READ_MULTIPLE   0xC4
#define CMD_WRITE_MULTIPLE  0xC5
#define CMD_IDENTIFY        0xEC
#define CMD_FLUSH           0xE7
#define CMD_MULTIPLE_MODE   0xc6

#define LBA_DRIVE0          0xE0  // Drive/Head 寄存器 4-7位,选择 lba模式,0主盘

struct ide_device ide_dev;
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


// size 为扇区数
INLINE void read_sector(void *buf, int size) {
    insw(IDE_DAT, buf, size * SECTOR_SIZE / 2);
}

INLINE void write_sector(void *b, int size) {
    outsw(IDE_DAT, b, size * SECTOR_SIZE / 2);
}


void ide_init() {
    assertk(ide_wait(0) == 0);

    // 选择要操作的设备为主盘, LBA 寻址
    outb(IDE_DH, LBA_DRIVE0);

    //发送 IDENTIFY 指令
    ide_send_cmd(CMD_IDENTIFY);

    // 检查设备是否存在
    assertk(inb(IDE_ALT_STAT) != 0 && ide_wait(1) == 0);

    // 读取IDE设备信息
    uint16_t buffer[256];
    read_sector(buffer, 1);

    ide_dev.lba48 = buffer[83] & (1 << 10);
    // TODO: 88/93  dma 检测

    ide_dev.size = buffer[60] | ((uint32_t) buffer[61] << 16);

    outb(IDE_COUNT, 8);
    ide_send_cmd(CMD_MULTIPLE_MODE);

    //开启磁盘中断
    outb(IDE_CTR, 0);

}

// 等待主盘可用
int32_t ide_wait(bool check_error) {
    uint8_t r;
    while ((r = inb(IDE_STAT)) & IDE_STAT_BSY);

    if (check_error && (r & (IDE_STAT_DF | IDE_STAT_ERR)) != 0) {
        return -1;
    }
    return 0;
}


void ide_isr(struct page*buf, bool write) {
    if (!write) {
        read_sector(buf->data, 8);
    }
}


void ide_driver_init(struct page*buf, uint32_t secs_cnt) {
    uint32_t no_sec = buf->pageCache.no_secs;
    // 选择 ide 驱动, 发送 lba 值, 设置扇区数
    assertk(ide_wait(0) == 0);

    // 选择 drive0
    outb(IDE_DH, LBA_DRIVE0 | ((no_sec >> 24) & MASK_U8(4)));
    // 需要读取的扇区数
    outb(IDE_COUNT, secs_cnt);
    // 写入 28 位 lba 值
    outb(IDE_NUM, no_sec & MASK_U8(8));
    outb(IDE_CYL_L, (no_sec >> 8) & MASK_U8(8));
    outb(IDE_CYL_H, (no_sec >> 16) & MASK_U8(8));
}

void ide_start(struct page*buf, bool write) {
    ide_driver_init(buf, 8);
    if (write) {
        ide_send_cmd(CMD_WRITE_MULTIPLE);
        write_sector(buf->data, 8);
    } else {
        ide_send_cmd(CMD_READ_MULTIPLE);
    }
    // 触发一次中断
    outb(IDE_CTR, 0);
}
