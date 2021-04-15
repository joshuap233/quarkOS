//
// Created by pjs on 2021/4/9.
//

#include "types.h"
#include "x86.h"
#include "drivers/pci.h"
#include "lib/qlib.h"

// 32 bit 端口
#define CONFIG_ADDRESS    0xCF8
#define CONFIG_DATA       0xCFC

#define INVALID_VENDOR    0xFFFF
//header_type 最高位被设置,则表示该设备有多个功能
#define MULTI_FUNC  (1 << 7)

// 配置空间偏移
#define VENDOR_OFFSET     0
#define HD_TYPE_OFFSET    0xe
#define CLASS_OFFSET      0xa
#define PRG_IF_OFFSET     0x9

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


// 使用 dev 的 class_code 与 subclass 探测设备
int8_t pci_device_detect(pci_dev_t *dev) {
    struct pci_cfg *cfg = &dev->cfg.field;
    cfg->func_no = 0;
    cfg->zero = 0;
    cfg->enable = 1;

    for (cfg->bus_no = 0; cfg->bus_no <= 255; cfg->bus_no++) {
        for (cfg->dev_no = 0; cfg->dev_no < 32; cfg->dev_no++) {
            uint8_t func_num = 1;
            if (pci_inw(dev, VENDOR_OFFSET) == INVALID_VENDOR) continue;
            uint8_t hd_type = pci_inb(dev, HD_TYPE_OFFSET);

            if (hd_type & MULTI_FUNC) func_num = 8;

            for (; cfg->func_no < func_num; cfg->func_no++) {
                uint16_t class_code = pci_inw(dev, CLASS_OFFSET);
                if (class_code == ((dev->class_code << 8) | dev->subclass)) {
                    dev->hd_type = pci_inb(dev, HD_TYPE_OFFSET);
                    dev->prg_if = pci_inb(dev, PRG_IF_OFFSET);
                    return 0;
                }
            }
        }
    }
    return -1;
}
