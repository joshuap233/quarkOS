//
// Created by pjs on 2021/5/4.
//
#include "mm/kmalloc.h"
#include "mm/page_alloc.h"
#include "mm/slab.h"
#include "types.h"
#include "mm/vmm.h"


void *kmalloc(u32_t size) {
    if (size < SLAB_MAX)
        return slab_alloc(size);
    ptr_t addr = pm_alloc(size);
    if (!IS_POWER_OF_2(size))
        size = fixSize(size);
    vmm_mapd(addr, addr, size, VM_PRES | VM_KW);
    return (void *) addr;
}


void kfree(void *addr) {
    ptr_t mem = (ptr_t) addr;
    // slabInfo 结构在页头,所以 slab 分配的地址不会页对齐
    if (mem & PAGE_MASK)
        return slab_free(addr);
    u32_t size = pm_free((ptr_t) addr);
    vmm_unmap((void *) addr, size);
}