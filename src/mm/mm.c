//
// Created by pjs on 2021/2/1.
//

#include "mm.h"
#include "multiboot2.h"
#include "qlib.h"
#include "physical_mm.h"
#include "virtual_mm.h"

void mm_init() {
    parse_memory_map(phymm_init);
    vmm_init();
//    printfk("%x\n", (uint32_t) _startKernel);
}
