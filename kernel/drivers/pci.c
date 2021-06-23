//
// Created by pjs on 2021/4/9.
//

#include <types.h>
#include <x86.h>
#include <drivers/pci.h>
#include <lib/qlib.h>

// 32 bit 端口
#define CONFIG_ADDRESS    0xCF8
#define CONFIG_DATA       0xCFC

#define INVALID_VENDOR    0xFFFF
//header_type 最高位被设置,则表示该设备有多个功能
#define MULTI_FUNC        (1 << 7)

// 配置空间偏移(字节)
#define VENDOR_OFFSET     0x0
#define DEV_OFFSET        0x2
#define HD_TYPE_OFFSET    0xe
#define CLASS_OFFSET      0xa
#define PRG_IF_OFFSET     0x9

//base address 在配置空间的偏移
#define PCI_BA_OFFSET0    0x10
#define PCI_BA_OFFSET1    0x14
#define PCI_BA_OFFSET2    0x18
#define PCI_BA_OFFSET3    0x1c
#define PCI_BA_OFFSET4    0x20
#define PCI_BA_OFFSET5    0x24

static u32_t map[] = {
        PCI_BA_OFFSET0,
        PCI_BA_OFFSET1,
        PCI_BA_OFFSET2,
        PCI_BA_OFFSET3,
        PCI_BA_OFFSET4,
        PCI_BA_OFFSET5
};

// 在配置空间偏移为 offset 处读取4字节
uint32_t pci_inl(pci_dev_t *dev, uint8_t offset) {
    dev->cfg.field.offset = offset & 0xfc;
    outl(CONFIG_ADDRESS, dev->cfg.data);
    return inl(CONFIG_DATA);
}

void pci_outl(pci_dev_t *dev, uint8_t offset, uint32_t value) {
    dev->cfg.field.offset = offset & 0xfc;
    outl(CONFIG_ADDRESS, dev->cfg.data);
    outl(CONFIG_DATA, value);
}

uint8_t pci_inb(pci_dev_t *dev, uint8_t offset) {
    return (uint8_t) ((pci_inl(dev, offset) >> ((offset & 3) * 8)) & MASK_U8(8));
}

uint16_t pci_inw(pci_dev_t *dev, uint8_t offset) {
    return (uint16_t) ((pci_inl(dev, offset) >> ((offset & 2) * 8)) & MASK_U16(16));
}

u32_t pci_read_bar(pci_dev_t *pci, int8_t no) {
    assertk(no <= 5);
    return pci_inl(pci, map[no]);
}

void pci_write_bar(pci_dev_t *pci, int8_t no, u32_t value) {
    assertk(no <= 5);
    pci_outl(pci, map[no], value);
}

// 使用 dev 的 class_code 与 subclass 探测设备
int8_t pci_device_detect(pci_dev_t *dev) {
    struct pci_cfg *cfg = &dev->cfg.field;
    cfg->zero = 0;
    cfg->enable = 1;

    for (u32_t bus_no = 0; bus_no <= 255; bus_no++) {
        for (u32_t dev_no = 0; dev_no < 32; dev_no++) {
            u8_t func_num = 1;
            cfg->bus_no = bus_no;
            cfg->dev_no = dev_no;
            cfg->func_no = 0;
            if (pci_inw(dev, VENDOR_OFFSET) == INVALID_VENDOR) continue;
            u8_t hd_type = pci_inb(dev, HD_TYPE_OFFSET);
            if (hd_type & MULTI_FUNC) func_num = 8;

            for (u32_t func_no = 0; func_no < func_num; func_no++) {
                cfg->func_no = func_no;
                u16_t class_code = pci_inw(dev, CLASS_OFFSET);
                if (class_code == ((dev->class_code << 8) | dev->subclass)) {
                    dev->hd_type = pci_inb(dev, HD_TYPE_OFFSET);
                    dev->prg_if = pci_inb(dev, PRG_IF_OFFSET);

                    dev->vendor_id = pci_inw(dev, VENDOR_OFFSET);
                    dev->dev_id = pci_inw(dev, DEV_OFFSET);
                    return 0;
                }
            }
        }
    }
    return -1;
}
