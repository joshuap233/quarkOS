//
// Created by pjs on 2021/6/18.
//
// https://wiki.osdev.org/Intel_Ethernet_i217
// 硬件手册: https://pdos.csail.mit.edu/6.828/2011/readings/hardware/8254x_GBe_SDM.pdf


#include <drivers/nic.h>
#include <drivers/pci.h>
#include <types.h>
#include <x86.h>
#include <lib/qlib.h>
#include <mm/kvm.h>
#include <isr.h>
#include <net/eth.h>


#define INTEL_VEND     0x8086  // Vendor ID for Intel
#define E1000_DEV      0x100E  // Device ID for the e1000 Qemu, Bochs, and VirtualBox emmulated NICs
#define E1000_I217     0x153A  // Device ID for Intel I217
#define E1000_82577LM  0x10EA  // Device ID for Intel 82577LM

/**  Ethernet Controller Register  **/
#define REG_CTRL        0x0000
#define REG_STATUS      0x0008
#define REG_EEPROM      0x0014  // EEPROM Read
#define REG_CTRL_EXT    0x0018  // Extended Device Control

// 中断
#define REG_ICR         0x00C0  // 读取中断发生的原因
#define REG_ITR         0x00c4
#define REG_ICS         0x00c8  // 用于设置中断条件
#define REG_IMS         0x00D0  // 设置来触发相应的中断
#define REG_IMC         0x00d8  // 设置来禁用相应的中断


// Interrupt Cause Read Register(icr):
#define ICR_LSC         (1<<2)  // Link Status Change
#define ICR_RXDMT0      (1<<4)
#define ICR_RXT0        (1<<7)

#define REG_RCTRL       0x0100  // Receive Control
#define REG_RXDESCLO    0x2800  // Receive Descriptor Base Low
#define REG_RXDESCHI    0x2804  // Receive Descriptor Base High
#define REG_RXDESCLEN   0x2808  // Receive Descriptor Length
#define REG_RXDESCHEAD  0x2810  // Receive Descriptor Head
#define REG_RXDESCTAIL  0x2818  // Receive Descriptor Tail

#define REG_TCTRL       0x0400  // Transmit Control
#define REG_TXDESCLO    0x3800  // Transmit Descriptor Base Low
#define REG_TXDESCHI    0x3804  // Transmit Descriptor Base High
#define REG_TXDESCLEN   0x3808  // Transmit Descriptor Length
#define REG_TXDESCHEAD  0x3810  // Transmit Descriptor Head
#define REG_TXDESCTAIL  0x3818  // Transmit Descriptor Tail


#define REG_RDTR           0x2820 // Receive Delay Timer
#define REG_RXDCTL         0x3828 // Transmit Descriptor Control
#define REG_RADV           0x282C // Receive Interrupt Absolute Delay Time
#define REG_RSRPD          0x2C00 // Receive Small Packet Detect Interrup

#define REG_TIPG           0x0410 // Transmit Inter Packet Gap

#define REG_MTA            0x5200 // Multicast Table Array
#define REG_MTA_NUM        128


#define REG_RAL            0x5400 // Receive Address Low (n)
#define REG_RAH            0x5404 // Receive Address High (n)

/* Device Control Register */
#define CTRL_FD           1           // Full-Duplex
#define CTRL_ASDE         (1 << 5)    // Auto-Speed Detection Enable.
#define CTRL_SLU          (1 << 6)    // set link up
#define CTRL_ILOS         (1 << 7)    // Invert Loss-of-Signal (LOS).
#define CTRL_FRCSPD       (1 << 11)   // Force Speed
#define CTRL_FRCDPLX      (1 << 12)   // Force Duplex



/* Receive CTL Register */
#define RCTL_EN            (1 << 1)    // Receiver Enable
#define RCTL_SBP           (1 << 2)    // Store Bad Packets
#define RCTL_UPE           (1 << 3)    // Unicast Promiscuous Enabled
#define RCTL_MPE           (1 << 4)    // Multicast Promiscuous Enabled
#define RCTL_LPE           (1 << 5)    // Long Packet Reception Enable
#define RCTL_LBM_NONE      (0 << 6)    // No Loopback
#define RCTL_LBM_PHY       (3 << 6)    // PHY or external SerDesc loopback
#define RTCL_RDMTS_HALF    (0 << 8)    // 可用缓冲区小于 1/2 RDLEN 时触发中断
#define RTCL_RDMTS_QUARTER (1 << 8)    // Free Buffer Threshold is 1/4 of RDLEN
#define RTCL_RDMTS_EIGHTH  (2 << 8)    // Free Buffer Threshold is 1/8 of RDLEN
#define RCTL_MO_36         (0 << 12)   // Multicast Offset - bits 47:36
#define RCTL_MO_35         (1 << 12)   // Multicast Offset - bits 46:35
#define RCTL_MO_34         (2 << 12)   // Multicast Offset - bits 45:34
#define RCTL_MO_32         (3 << 12)   // Multicast Offset - bits 43:32
#define RCTL_BAM           (1 << 15)   // Broadcast Accept Mode
#define RCTL_VFE           (1 << 18)   // VLAN Filter Enable
#define RCTL_CFIEN         (1 << 19)   // Canonical Form Indicator Enable
#define RCTL_CFI           (1 << 20)   // Canonical Form Indicator Bit Value
#define RCTL_DPF           (1 << 22)   // Discard Pause Frames
#define RCTL_PMCF          (1 << 23)   // Pass MAC Control Frames

/* Controls whether the hardware strips the Ethernet CRC from the received packet.
 * This stripping occurs prior to any checksum calculations. The stripped CRC is n
 * ot transferred to host memory and is not included in the length reported in the descriptor
 */
#define RCTL_SECRC         (1 << 26)   // Strip Ethernet CRC

// Buffer Sizes
// 使用 bit 16-17, bit 25 设置接收缓冲区大小
#define RCTL_BSIZE_256     (3 << 16)
#define RCTL_BSIZE_512     (2 << 16)
#define RCTL_BSIZE_1024    (1 << 16)
#define RCTL_BSIZE_2048    (0 << 16)
#define RCTL_BSIZE_4096    ((3 << 16) | (1 << 25))
#define RCTL_BSIZE_8192    ((2 << 16) | (1 << 25))
#define RCTL_BSIZE_16384   ((1 << 16) | (1 << 25))


// Transmit Command
#define CMD_EOP          (1 << 0)    // End of Packet
#define CMD_IFCS         (1 << 1)    // Insert FCS
#define CMD_IC           (1 << 2)    // Insert Checksum
#define CMD_RS           (1 << 3)    // Report Status
#define CMD_RPS          (1 << 4)    // Report Packet Sent
#define CMD_VLE          (1 << 6)    // VLAN Packet Enable
#define CMD_IDE          (1 << 7)    // Interrupt Delay Enable


/* Transmit CTL Register */
#define TCTL_EN          (1 << 1)    // Transmit Enable
#define TCTL_PSP         (1 << 3)    // Pad Short Packets
#define TCTL_CT_SHIFT    4           // Collision Threshold
#define TCTL_COLD_SHIFT  12          // Collision Distance
#define TCTL_SWXOFF      (1 << 22)   // Software XOFF Transmission
#define TCTL_RTLC        (1 << 24)   // Re-transmit on Late Collision

#define TSTA_DD          (1 << 0)    // Descriptor Done
#define TSTA_EC          (1 << 1)    // Excess Collisions
#define TSTA_LC          (1 << 2)    // Late Collision
#define LSTA_TU          (1 << 3)    // Transmit Underrun

#define IPGT             10          // 固定值
#define IPGT1            (8<<10)
#define IPGT2            (6<<20)


#define E1000_NUM_RX_DESC 32
#define E1000_NUM_TX_DESC 8

#define MAC_LENGTH 6
#define NIC_BUF_SIZE   (PAGE_SIZE*2)

#define RECV_DD         1       // Descriptor Done
#define RECV_EOP        (1<<1)  // End of Packet
#define RECV_IXSM       (1<<2)  // Ignore Checksum Indication(ipcs,tcpcs)
#define RECV_VP         (1<<3)  // 1b: the hardware performed the TCP/UDP checksum on the received packet.
#define RECV_TPCS       (1<<5)
#define RECV_IPCS       (1<<6)  //  // 1b: the hardware performed IP checksum
#define RECV_PIF        (1<<7)


struct recv_errors {
    volatile u8_t ce: 1;     // CRC Error or Alignment Error
    volatile u8_t se: 1;     // Symbol Error
    volatile u8_t seq: 1;    // Sequence Error
    volatile u8_t rsv: 1;    // Reserved
    volatile u8_t cxe: 1;
    volatile u8_t tcpe: 1;   // TCP/UDP Checksum Error
    volatile u8_t ipe: 1;    //IP Checksum Error
    volatile u8_t rxe: 1;    // RX Data Error
};

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
    uint16_t rx_cur;      // 当前接收描述符指针
    uint16_t tx_cur;
    u8_t isr_line;
} net_dev;

static u8_t rx_buf[E1000_NUM_RX_DESC][NIC_BUF_SIZE];

static void detectEEProm();

static u32_t eepromRead(uint8_t addr);

static void readMACAddress();

static void rx_init();

static void tx_init();

static void receivePacket();

static void nic_enableInterrupt();

u8_t read8(void *address);

u16_t read16(void *address);

u32_t read32(void *address);

u64_t read64(void *address);

void write8(void *address, u8_t value);

void write16(void *address, u16_t value);

void write32(void *address, u32_t value);

void write64(void *address, u64_t value);

void writeCommand(u16_t address, u32_t value);

INT nic_isr(UNUSED interrupt_frame_t *frame);


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

    // start link
    u32_t cfg = read32(net_dev.va + REG_CTRL);
    write32(net_dev.va + REG_CTRL, cfg | CTRL_SLU);


    // Multicast Table Array 初始化, 128 个 32 位寄存器
    // 用于过滤多播包,见 82540EM 手册 13.5.1
    for (int i = 0; i < REG_MTA_NUM; i++)
        writeCommand(REG_MTA + i * 4, 0);

//    net_dev.isr_line = pci_interrupt_line(&net_dev.pci_dev);
//    reg_isr(IRQ0 + net_dev.isr_line, nic_isr);
    // TODO:中断

//    Program the Interrupt Mask Set/Read (IMS)
//    register to enable any interrupt the software driver
//    wants to be notified of when the event occurs.
//    Suggested bits include RXT, RXO, RXDMT, RXSEQ, and LSC.
//    There is no immediate reason to enable the transmit interrupts.
//    If software uses the Receive Descriptor Minimum Threshold Interrupt,
//    the Receive Delay Timer (RDTR) register should be initialized with the desired delay time.

//    nic_enableInterrupt();

    rx_init();
    tx_init();

    // REG_TIPG 设置帧发送时间间隙
    writeCommand(REG_TIPG, IPGT | IPGT1 | IPGT2);
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
        u8_t *mac_8 = net_dev.va + REG_RAL;
        u32_t *mac_32 = net_dev.va + REG_RAL;
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
    net_dev.rx_cur = 0;

    // 设置描述符基址
    writeCommand(REG_RXDESCLO, ptr);
    writeCommand(REG_RXDESCHI, 0);

    // 设置描述符总长度(字节),长度必须 128 字节对齐
    writeCommand(REG_RXDESCLEN, E1000_NUM_RX_DESC * sizeof(struct e1000_rx_desc));

    // 描述符缓存队列头指针
    writeCommand(REG_RXDESCHEAD, 0);

    // tail should point to one descriptor beyond the
    // last valid descriptor in the descriptor ring.
    // 因此第一个被使用的描述符为 0 号
    writeCommand(REG_RXDESCTAIL, E1000_NUM_RX_DESC - 1);

    // RCTL_BSIZE_8192 设置接收缓冲区大小为 8192
    // RCTL_UPE 开启混杂模式
    // RCTL_LBM_NONE 设置不使用本地回环
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
    net_dev.tx_cur = 0;

    writeCommand(REG_TXDESCHI, ptr);
    writeCommand(REG_TXDESCLO, 0);

    writeCommand(REG_TXDESCLEN, E1000_NUM_TX_DESC * sizeof(struct e1000_tx_desc));

    writeCommand(REG_TXDESCHEAD, 0);
    writeCommand(REG_TXDESCTAIL, 0);

    // TCTL_PSP: Pad Short Packets
    // Padding makes the packet 64 bytes long. The padding content is data
    // When the Pad Short Packet feature is disabled,
    // the minimum packet size the Ethernet controller can transfer to the host is 32 bytes long
    // TCTL_CT_SHIFTL:  only has meaning in half duplex mode
    writeCommand(REG_TCTRL,
                 TCTL_EN
                 | TCTL_PSP
                 | (15 << TCTL_CT_SHIFT)
                 | (64 << TCTL_COLD_SHIFT)
                 | TCTL_RTLC
    );
}

static void nic_enableInterrupt() {
    writeCommand(REG_IMS, 0x1F6DC);
    writeCommand(REG_IMS, 0xff & ~4);
    readCommand(0xc0);
}

INT nic_isr(UNUSED interrupt_frame_t *frame) {
    /* This might be needed here if your handler doesn't clear interrupts from each device and must be done before EOI if using the PIC.
       Without this, the card will spam interrupts as the int-line will stay high. */
    writeCommand(REG_IMS, 0x1);

    uint32_t status = readCommand(REG_ICR);
    if (status & ICR_LSC) {
        int debug = 1;
//        startLink();
    } else if (status & ICR_RXDMT0) {
        int debug = 2;
        // good threshold
    } else if (status & ICR_RXT0) {
        receivePacket();
    }
    pic2_eoi(IRQ0 + net_dev.isr_line);
}


void receivePacket() {
    uint16_t old_cur;

    while ((net_dev.rx_desc[net_dev.rx_cur].status & RECV_DD)) {
        uint8_t *buf = (uint8_t *) net_dev.rx_desc[net_dev.rx_cur].addr;
        uint16_t len = net_dev.rx_desc[net_dev.rx_cur].length;

        // Here you should inject the received packet into your network stack

        net_dev.rx_desc[net_dev.rx_cur].status = 0;
        old_cur = net_dev.rx_cur;
        net_dev.rx_cur = (net_dev.rx_cur + 1) % E1000_NUM_RX_DESC;
        writeCommand(REG_RXDESCTAIL, old_cur);
    }
}

int sendPacket(const void *p_data, uint16_t p_len) {
    assertk(p_len <= NIC_BUF_SIZE);

    net_dev.tx_desc[net_dev.tx_cur].addr = (uint64_t) p_data;
    net_dev.tx_desc[net_dev.tx_cur].length = p_len;
    net_dev.tx_desc[net_dev.tx_cur].cmd = CMD_EOP | CMD_IFCS | CMD_RS;
    net_dev.tx_desc[net_dev.tx_cur].status = 0;
    uint8_t old_cur = net_dev.tx_cur;
    net_dev.tx_cur = (net_dev.tx_cur + 1) % E1000_NUM_TX_DESC;
    writeCommand(REG_TXDESCTAIL, net_dev.tx_cur);
    while (!(net_dev.tx_desc[old_cur].status & 0xff));
    return 0;
}
