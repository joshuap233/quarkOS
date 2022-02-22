//
// Created by pjs on 2022/2/12.
//
#include <multiboot2.h>
#include <lib/qlib.h>
#include <lib/qstring.h>
#include <highmem.h>
#include "drivers/acpi.h"
#include <mm/vm.h>

#define RSDP_SIGNATURE "RSD PTR "
#define RSDT_SIGNATURE "RSDT"

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
    struct sdtHeader *rsdt = (struct sdtHeader *) (addr + KERNEL_START);

    if (!memcmp(rsdt, RSDT_SIGNATURE, 4)) {
        return NULL;
    }
    if (!acpiChecksum(rsdt)) {
        return NULL;
    }
    return rsdt;
}

void acpi_init() {
    struct rsdpDescriptor *rsdp = get_rsdp();
    assertk(rsdp);

    kvm_maps(PAGE_CEIL(rsdp->rsdtAddress+KERNEL_START),
            rsdp->rsdtAddress,
            PAGE_SIZE,VM_KR|VM_PRES);

    struct sdtHeader *rsdt = get_rsdt(rsdp->rsdtAddress);
    // rsdt 后跟随 sdt 指针
    assertk(rsdt && rsdt->length <= PAGE_SIZE);

    u32_t *entry = (void *) rsdt + sizeof(struct sdtHeader);
    u32_t cnt = (rsdt->length - sizeof(struct sdtHeader)) / 4;
    for (u32_t i = 0; i < cnt; i++) {
        struct sdtHeader*e = (struct sdtHeader *)(*entry + KERNEL_START);
        if (memcmp(e, "APIC", 4)) {
            sysDesTable.madt = e;
        }
        entry++;
    }

//    kvm_unmap3((void*)PAGE_CEIL(rsdp->rsdtAddress+KERNEL_START),PAGE_SIZE);
}

