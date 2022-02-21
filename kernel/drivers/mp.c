//
// Created by pjs on 2022/2/12.
//
#include <types.h>
#include <limit.h>
#include <drivers/acpi.h>
#include <lib/qstring.h>
#include <lib/qlib.h>
#include <cpu.h>
#include <highmem.h>
#include <drivers/mp.h>
#include <drivers/lapic.h>
#include <mm/vm.h>

#define LAPIC_ENTRY   0
#define IOAPIC_ENTRY  1


struct cpu cpus[N_CPU] = {
        [0 ...N_CPU-1] = {
                .ir_enable = true,
                .idle = NULL,
                .lock = NULL,
                .apic_id = -1,
                .start = false
        }
};

struct apic cpuCfg = {
        .nCpu = 0,
        .ioApicId = 0,
        .lapicPtr = 0,
        .ioapicPtr = 0
};

struct apicTable {
    struct sdtHeader header;
    u32_t lapicAddr;
    u32_t flags;
};


// entry 头
struct apicEntry {
    u8_t entryType;
    u8_t length;
};

struct lapicEntry {
    struct apicEntry apicEntry;
    u8_t acpiProcessorId;
    u8_t apicId;
    u32_t flags;
};

struct ioapicEntry {
    struct apicEntry apicEntry;
    u8_t ioapicId;
    u8_t reserved;
    u32_t ioapicAddr;
    u32_t gsib;      //Global System Interrupt Base
};

// 启用 apic 时禁用 pic
extern void disable_pic();

void smp_init() {
    disable_pic();

    struct apicTable *table = sysDesTable.madt;

    assertk(memcmp(table, "APIC", 4));
    assertk(acpiChecksum(&table->header));

    struct lapicEntry *lapicEntry;
    struct ioapicEntry *ioapicEntry;

    cpuCfg.lapicPtr = table->lapicAddr;
    for (struct apicEntry *entry = (struct apicEntry *) (table + 1),
                 *end = (void *) table + table->header.length;
         entry < end; entry = (void *) entry + entry->length) {
        switch (entry->entryType) {
            case LAPIC_ENTRY:
                lapicEntry = (struct lapicEntry *) entry;
                if (lapicEntry->flags & 1) {
                    cpus[cpuCfg.nCpu++].apic_id = lapicEntry->apicId;
                }
                break;
            case IOAPIC_ENTRY:
                //TODO: 可能有多个 apic
                if (cpuCfg.ioapicPtr == 0) {
                    ioapicEntry = (struct ioapicEntry *) entry;
                    cpuCfg.ioApicId = ioapicEntry->ioapicId;
                    cpuCfg.ioapicPtr = ioapicEntry->ioapicAddr;
                }
                break;
        }
    }

}


void cpu_init(){
    for (int i = 0; i < cpuCfg.nCpu; ++i) {
        cpus[i].ir_enable = true;
    }
    struct cpu*cpu = getCpu();
    assertk(cpu->lock == NULL);
    assertk(cpu->apic_id>=0);
}

struct cpu *getCpu() {
    u32_t id = lapicId();

    for (int i = 0; i < cpuCfg.nCpu; ++i) {
        if (cpus[i].apic_id == (int8_t)id){
            return &cpus[i];
        }
    }
    assertk(false);
    return NULL;
}
