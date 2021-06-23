//
// Created by pjs on 2021/4/9.
//

#ifndef QUARKOS_DRIVERS_PCI_H
#define QUARKOS_DRIVERS_PCI_H

#include <types.h>


#define PCI_MEM_BAR(bar)          (((bar) & 1) == 0)
#define PCI_BAR_TYPE(bar)         ((bar) & 1)
#define PCI_MEM_BAR_ADDR(bar)     ((bar) & (MASK_U32(28) << 4))
#define PCI_MEM_BAR_PREFETCH(bar) ((bar) & (0b1 << 3))
#define PCI_MEM_BAR_TYPE(bar)     ((bar) & (0b11 << 1))

#define PCI_IO_BAR_ADDR(bar)      ((bar) & (MASK_U32(30) << 2))


typedef union {
    struct pci_cfg {
        u8_t offset;
        u8_t func_no: 3;
        u8_t dev_no: 5;
        u8_t bus_no;
        u8_t zero: 7;
        u8_t enable: 1;
    } field;
    u32_t data;
} cfg_addr_t;


typedef struct pci_dev {
    u16_t vendor_id;
    u16_t dev_id;

    u8_t class_code;
    u8_t subclass;
    u8_t prg_if;  //Programming Interface
    u8_t hd_type: 7;  // header type
    u8_t : 1;
    cfg_addr_t cfg;
} pci_dev_t;

int8_t pci_device_detect(pci_dev_t *dev);

uint16_t pci_inw(pci_dev_t *dev, uint8_t offset);

uint8_t pci_inb(pci_dev_t *dev, uint8_t offset);

uint32_t pci_inl(pci_dev_t *dev, uint8_t offset);

void pci_outl(pci_dev_t *dev, uint8_t offset, uint32_t value);

u32_t pci_read_bar(pci_dev_t *pci, int8_t no);

void pci_write_bar(pci_dev_t *pci, int8_t no, u32_t value);

#endif //QUARKOS_DRIVERS_PCI_H
