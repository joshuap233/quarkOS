//
// Created by pjs on 2022/2/10.
// 参考 x86 多处理器规范(MPS)
#include <types.h>
#include "drivers/mp.h"
#include <highmem.h>
#include <lib/qstring.h>
#include <lib/qlib.h>
#include <cpu.h>
#include <limit.h>
#include <x86.h>

// 见 BDA 内存布局
#define EBDA_BA 0x040E
#define BM_SIZE 0x0413
#define BIO_ROM_START 0xf0000
#define BIO_ROM_LEN   0x10000


#define IMCR_ADDR 0x22
#define IMCR_DATA 0x23

struct cpu cpus[N_CPU];
u8_t nCpu;
u8_t ioApicId;



#define FPS_SIGNATURE           "_MP_"
#define CFG_TABLE_SIGNATURE     "PCMP"

// floating Pointer Structures
struct fps {
    u8_t signature[4];           // _MP_
    u32_t ct;                    // MP configuration table 物理地址
    u8_t length;                 // 1
    u8_t version;                // 1/4
    u8_t checksum;               // 使结构字段值和为 0

    u8_t type;
    // 如果为 0 则忽略 MP configuration table,
    // 使用规范定义的默认配置

    u32_t features;
};

// configuration table header
struct cfgTable {
    char signature[4];            // PCMP
    u16_t length;
    u8_t version;                 // 1/4
    u8_t checksum;                // 使结构字段值和为 0
    char oem_id[8];               // 厂商 id
    char product_id[12];          // 产品家族 id
    u32_t oem_table;              // oem-defined config table 物理地址或 0
    u16_t oem_table_size;
    u16_t entry_count;
    u32_t lapic_address;          // lapic mmio 地址
    u16_t extendedTableLength;
    u8_t extendedTableChecksum;
    u8_t reserved;
};

// configuration table entry types
#define PROC_ENTRY    0
#define BUS_ENTRY     1
#define IO_APIC_ENTRY 2
#define IO_IR_ENTRY   3
#define L_IR_ENTRY    4

// configuration table entry 的一种
struct processorEntry {
    u8_t type;           // 0
    u8_t localApicId;
    u8_t localApicVersion;
    u8_t flags;
    // bit 0 置 0 则当前处理器不可用
    // bit 1 值 1 则当前处理器为 bootstrap processor(BSP)

    u32_t signature;
    u32_t featureFlags;
    u64_t reserved;
};


struct ioApicEntry {
    u8_t type;     // 2
    u8_t id;       // IO APIC id
    u8_t ioapicVersion;
    u8_t flags;    // 值 0 则当前 io APIC 不可用
    u32_t address; // IO APIC mmio 地址
};


static u8_t sum(const u8_t *addr, int32_t len) {
    u8_t sum=0;
    for (int i = 0; i < len; ++i) {
        sum += addr[i];
    }
    return sum;
}


static struct fps *search(u32_t addr, int32_t len) {
    for (struct fps *fps = addr + HIGH_MEM,
                 *end = (void *) fps + len;
         fps < end; fps++) {
        if (memcmp(fps, FPS_SIGNATURE, 4) &&
            sum(fps, sizeof(struct fps))) {
            return fps;
        }
    }
    return NULL;
}

// 搜索 floating Pointer Structures
// 依次搜索
// 1. EBDA 前 1000 字节
// 2. base memory 最后一千字节(就是 ebda 前面那块)
// 3. 0xf0000 - 0xfffff
static struct fps *search_fps() {
    u32_t a = *((u64_t *) EBDA_BA) << 4;
    u32_t b = *((u64_t *) BM_SIZE) * K;

    struct fps *mp;
    if (mp = search(a, K)) {
        return mp;
    }
    if (mp = search(b - K, K)) {
        return mp;
    }

    return search(BIO_ROM_START, BIO_ROM_LEN);
}


static struct cfgTable *search_cfgTable(struct fps *fps) {
    struct cfgTable *cfg;

    if (!(fps->ct && fps->type)) {
        return NULL;
    }
    if (fps->version != 1 && fps->version != 4) {
        return NULL;
    }

    cfg = (void *) fps->ct + HIGH_MEM;
    if (!memcmp(cfg, CFG_TABLE_SIGNATURE, 4)) {
        return NULL;
    }
    if (cfg->version != 1 && cfg->version != 4) {
        return NULL;
    }
    if (sum(cfg, cfg->length) != 0) {
        return NULL;
    }
    return cfg;
}


void smp_init() {
    struct fps *fps = search_fps();
    assertk(fps);
    struct cfgTable *cfg = search_cfgTable(fps);
    assertk(cfg);

    struct processorEntry *proc;
    struct ioApicEntry *apic;

    // 搜索 configuration table entry
    for (u8_t *entry = cfg + 1,
                 *end = (u8_t *) cfg + cfg->length;
         entry < end;) {

        switch (*entry) {
            case PROC_ENTRY:
                proc = entry;
                if (nCpu < N_CPU) {
                    cpus[nCpu++].apic_id = proc->localApicId;
                }
                entry += sizeof(struct processorEntry);
                continue;
            case IO_APIC_ENTRY:
                apic = entry;
                ioApicId = apic->id;
                entry += sizeof(struct ioApicEntry);
                continue;
            case BUS_ENTRY:
            case IO_IR_ENTRY:
            case L_IR_ENTRY:
                entry += 8;
                continue;
            default:
                panic("smp_init error");
                break;
        }

    }

    outb(IMCR_ADDR, 0x70);
    outb(IMCR_DATA, 1);
}
