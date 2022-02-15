//
// Created by pjs on 2022/2/12.
//
#include <multiboot2.h>
#include <lib/qlib.h>
#include <lib/qstring.h>
#include <highmem.h>
#include "drivers/acpi.h"
#include <mm/kvm.h>

#define RSDP_SIGNATURE "RSD PTR "
#define RSDT_SIGNATURE "RSDP"

struct sysDesTable sysDesTable;

// acpi 1.0
struct rsdpDescriptor {
    char signature[8];
    u8_t checksum;
    u8_t oemId[6];
    u8_t version;
    u32_t rsdtAddress; // rdst 物理地址
} PACKED;


static u8_t sum(const u8_t *addr, u32_t len) {
    u8_t sum = 0;
    for (u32_t i = 0; i < len; ++i) {
        sum += addr[i];
    }
    return sum;
}


bool acpiChecksum(struct sdtHeader *header) {
    return sum((u8_t *) header, header->length) == 0;
}


static struct rsdpDescriptor *get_rsdp() {
    struct rsdpDescriptor *rsdp = bInfo.rsdp;
    if (!rsdp) {
        return NULL;
    }
    if (!memcmp(rsdp, RSDP_SIGNATURE, 8)) {
        return NULL;
    }
    if (rsdp->version != 0) {
        return NULL;
    } // 1.0

    if (sum((u8_t *) rsdp, sizeof(struct rsdpDescriptor)) != 0) {
        return NULL;
    }
    return rsdp;
}

static struct sdtHeader *get_rsdt(u32_t addr) {
    struct sdtHeader *rsdt = (struct sdtHeader *) (addr + HIGH_MEM);

    if (!memcmp(rsdt, RSDT_SIGNATURE, 4)) {
        return NULL;
    }
    if (acpiChecksum(rsdt)) {
        return NULL;
    }
    return rsdt;
}

void acpi_init() {
    struct rsdpDescriptor *rsdp = get_rsdp();
    assertk(rsdp);
    kvm_maps(
            (u32_t) rsdp + HIGH_MEM,
            (u32_t) rsdp,
            sizeof(struct sdtHeader),
            VM_PRES | VM_KR
    );

    struct sdtHeader *rsdt = get_rsdt(rsdp->rsdtAddress);
    assertk(rsdt);

    u32_t *entry = (void *) rsdt + sizeof(struct sdtHeader);
    u32_t cnt = (rsdt->length - sizeof(struct sdtHeader)) / 4;
    for (u32_t i = 0; i < cnt; i++) {
        if (memcmp((void *) *entry, "APIC", 4)) {
            sysDesTable.madt = entry;
        }
        entry++;
    }

    kvm_unmap2(a);
}

