//
// Created by pjs on 2021/4/9.
//

#ifndef QUARKOS_DRIVERS_PCI_H
#define QUARKOS_DRIVERS_PCI_H
#include "types.h"


typedef union {
    struct pci_cfg{
        uint8_t offset;
        uint8_t func_no: 3;
        uint8_t dev_no: 5;
        uint8_t bus_no;
        uint8_t zero: 7;
        uint8_t enable: 1;
    } field;
    uint32_t data;
} cfg_addr_t;


typedef struct pci_dev {
//base address 在配置空间的偏移
#define PCI_BA_OFFSET0    0x10
#define PCI_BA_OFFSET1    0x14
#define PCI_BA_OFFSET2    0x18
#define PCI_BA_OFFSET3    0x1c
#define PCI_BA_OFFSET4    0x20
#define PCI_BA_OFFSET5    0x24
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prg_if;  //Programming Interface
    uint8_t hd_type: 7;  // header type
    uint8_t : 1;
    cfg_addr_t cfg;
} pci_dev_t;

int8_t pci_device_detect(pci_dev_t *dev);
uint16_t pci_inw(pci_dev_t *dev, uint8_t offset);
uint8_t pci_inb(pci_dev_t *dev, uint8_t offset);
uint32_t pci_inl(pci_dev_t *dev, uint8_t offset);
void pci_outl(pci_dev_t *dev, uint8_t offset, uint32_t value);


#endif //QUARKOS_DRIVERS_PCI_H
