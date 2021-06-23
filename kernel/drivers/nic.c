//
// Created by pjs on 2021/6/18.
//
// https://wiki.osdev.org/Intel_Ethernet_i217

#include <drivers/nic.h>
#include <drivers/pci.h>
#include <types.h>
#include <x86.h>
#include <lib/qlib.h>
#include <mm/kvm.h>

#define MAC_LENGTH 6

static struct net_dev {
    bool eeprom;
    u32_t bar_type;
    u32_t io_base;

    void *va;
    u32_t mem_base;
    u32_t mem_size;
    pci_dev_t pci_dev;
    u8_t mac[MAC_LENGTH];

    _Alignas(16) struct e1000_rx_desc rx_desc[E1000_NUM_RX_DESC];
    _Alignas(16) struct e1000_tx_desc tx_desc[E1000_NUM_TX_DESC];
    uint16_t rx_cur;      // Current Receive Descriptor Buffer
    uint16_t tx_cur;      // Current Transmit Descriptor Buffer
} net_dev;

// 没找到对齐要求,不确定是不是要对齐
static u8_t rx_buf[E1000_NUM_RX_DESC][PAGE_SIZE * 2];

static void detectEEProm();

static u32_t eepromRead(uint8_t addr);

static void readMACAddress();

static void rx_init();

static void tx_init();

void nic_init() {
    pci_dev_t *pciDev = &net_dev.pci_dev;
    pciDev->class_code = 0x02;
    pciDev->subclass = 0;
    assertk(pci_device_detect(pciDev) == 0);
    assertk(pciDev->hd_type == 0 || pciDev->prg_if != 128);
    assertk(pciDev->vendor_id == INTEL_VEND);
    assertk(pciDev->dev_id == E1000_DEV);

    u32_t bar = pci_read_bar(pciDev, 0);
    net_dev.bar_type = PCI_BAR_TYPE(bar);

    if (PCI_MEM_BAR(bar)) {
        net_dev.mem_base = PCI_MEM_BAR_ADDR(bar);
        assertk(net_dev.mem_base >= HIGH_MEM);
        assertk(PCI_MEM_BAR_TYPE(bar) == 0);

        pci_write_bar(pciDev, 0, MASK_U32(32));
        u32_t tmp = pci_read_bar(pciDev, 0);
        net_dev.mem_size = (~PCI_MEM_BAR_ADDR(tmp)) + 1;

        // 需要将 bar 重新写回
        pci_write_bar(pciDev, 0, bar);

        // 直接映射
        net_dev.va = (void *) net_dev.mem_base;
        kvm_maps(net_dev.mem_base, net_dev.mem_base, net_dev.mem_size, VM_PRES | VM_KW);
    } else {
        assertk(0);
        net_dev.io_base = PCI_IO_BAR_ADDR(bar);
    }

    detectEEProm();
    readMACAddress();

    rx_init();
    tx_init();
}


u8_t read8(void *address) {
    return *((volatile u8_t *) address);
}

u16_t read16(void *address) {
    return *((volatile u16_t *) address);
}

u32_t read32(void *address) {
    return *((volatile u32_t *) address);
}

u64_t read64(void *address) {
    return *((volatile u64_t *) address);
}

void write8(void *address, u8_t value) {
    *((volatile uint8_t *) address) = value;
}

void write16(void *address, u16_t value) {
    *((volatile uint16_t *) address) = value;
}

void write32(void *address, u32_t value) {
    *((volatile u32_t *) address) = value;
}

void write64(void *address, u64_t value) {
    *((volatile u64_t *) address) = value;
}


void writeCommand(u16_t address, u32_t value) {
    if (net_dev.bar_type == 0) {
        write32(net_dev.va + address, value);
    } else {
        outl(net_dev.io_base, address);
        outl(net_dev.io_base + 4, value);
    }
}

u32_t readCommand(u16_t address) {
    if (net_dev.bar_type == 0) {
        return read32(net_dev.va + address);
    } else {
        outl(net_dev.io_base, address);
        return inl(net_dev.io_base + 4);
    }
}

static void detectEEProm() {
    writeCommand(REG_EEPROM, 0x1);

    for (int i = 0; i < 1000 && !net_dev.eeprom; i++) {
        u32_t val = readCommand(REG_EEPROM);
        if (val & 0x10)
            net_dev.eeprom = true;
        else
            net_dev.eeprom = false;
    }
}

static u32_t eepromRead(uint8_t addr) {
    u16_t data;
    u32_t tmp;
    if (net_dev.eeprom) {
        writeCommand(REG_EEPROM, 1 | ((u32_t) (addr) << 8));
        while (!((tmp = readCommand(REG_EEPROM)) & (1 << 4)));
    } else {
        writeCommand(REG_EEPROM, 1 | ((u32_t) (addr) << 2));
        while (!((tmp = readCommand(REG_EEPROM)) & (1 << 1)));
    }
    data = (u16_t) ((tmp >> 16) & 0xFFFF);
    return data;
}


static void readMACAddress() {
    u8_t *mac = net_dev.mac;
    if (net_dev.eeprom) {
        uint32_t temp;
        temp = eepromRead(0);
        mac[0] = temp & 0xff;
        mac[1] = temp >> 8;
        temp = eepromRead(1);
        mac[2] = temp & 0xff;
        mac[3] = temp >> 8;
        temp = eepromRead(2);
        mac[4] = temp & 0xff;
        mac[5] = temp >> 8;
    } else {
        u8_t *mac_8 = net_dev.va + 0x5400;
        u32_t *mac_32 = net_dev.va + 0x5400;
        if (mac_32[0] != 0) {
            for (int i = 0; i < 6; i++) {
                mac[i] = mac_8[i];
            }
            return;
        };
        assertk(0);
    }
}

static void rx_init() {
    ptr_t ptr = kvm_vm2pm((ptr_t) &net_dev.rx_desc);

    for (int i = 0; i < E1000_NUM_RX_DESC; i++) {
        net_dev.rx_desc[i].addr = kvm_vm2pm((ptr_t) &rx_buf[i]);
        net_dev.rx_desc[i].status = 0;
    }

    writeCommand(REG_TXDESCLO, (uint32_t) ((uint64_t) ptr >> 32));
    writeCommand(REG_TXDESCHI, (uint32_t) ((uint64_t) ptr & 0xFFFFFFFF));

    writeCommand(REG_RXDESCLO, (uint64_t) ptr);
    writeCommand(REG_RXDESCHI, 0);

    writeCommand(REG_RXDESCLEN, E1000_NUM_RX_DESC * sizeof(struct e1000_rx_desc));

    writeCommand(REG_RXDESCHEAD, 0);
    writeCommand(REG_RXDESCTAIL, E1000_NUM_RX_DESC - 1);
    net_dev.rx_cur = 0;
    writeCommand(REG_RCTRL,
                 RCTL_EN | RCTL_SBP | RCTL_UPE | RCTL_MPE | RCTL_LBM_NONE |
                 RTCL_RDMTS_HALF | RCTL_BAM | RCTL_SECRC | RCTL_BSIZE_8192
    );

}

static void tx_init() {
    ptr_t ptr = kvm_vm2pm((ptr_t) &net_dev.tx_desc);
    for (int i = 0; i < E1000_NUM_TX_DESC; i++) {
        net_dev.tx_desc[i].addr = 0;
        net_dev.tx_desc[i].cmd = 0;
        net_dev.tx_desc[i].status = TSTA_DD;
    }

    writeCommand(REG_TXDESCHI, ((uint64_t) ptr >> 32));
    writeCommand(REG_TXDESCLO, ((uint64_t) ptr & MASK_U32(32)));


    //now setup total length of descriptors
    writeCommand(REG_TXDESCLEN, E1000_NUM_TX_DESC * sizeof(struct e1000_tx_desc));


    //setup numbers
    writeCommand(REG_TXDESCHEAD, 0);
    writeCommand(REG_TXDESCTAIL, 0);
    net_dev.tx_cur = 0;
    writeCommand(REG_TCTRL, TCTL_EN
                            | TCTL_PSP
                            | (15 << TCTL_CT_SHIFT)
                            | (64 << TCTL_COLD_SHIFT)
                            | TCTL_RTLC);

    // This line of code overrides the one before it but I left both to highlight that the previous one works with e1000 cards, but for the e1000e cards
    // you should set the TCTRL register as follows. For detailed description of each bit, please refer to the Intel Manual.
    // In the case of I217 and 82577LM packets will not be sent if the TCTRL is not configured using the following bits.
    writeCommand(REG_TCTRL, 0b0110000000000111111000011111010);
    writeCommand(REG_TIPG, 0x0060200A);

}

void nic_enableInterrupt() {
    writeCommand(REG_IMASK, 0x1F6DC);
    writeCommand(REG_IMASK, 0xff & ~4);
    readCommand(0xc0);

}
