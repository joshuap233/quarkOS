//
// Created by pjs on 2021/5/7.
//

#ifndef QUARKOS_MM_VMALLOC_H
#define QUARKOS_MM_VMALLOC_H

#include "types.h"

enum AREA_TYPE {
    KERNEL_AREA = 0, PAGE_CACHE_AREA = 1
};

struct vm_area {
    struct area {
        ptr_t addr;
        u32_t size;
        u32_t flag;
        struct area *next;
    } *area;
    struct vm_area *next;
    enum AREA_TYPE type;
};

void vm_area_add(ptr_t addr, u32_t size, u16_t flag, enum AREA_TYPE type);

void vm_area_expand(u32_t end_addr, enum AREA_TYPE type);

extern struct vm_area vm_area_head;

#define for_each_vm_area(hdr)     \
    for ((hdr) = vm_area_head.next; (hdr) != NULL; (hdr) = (hdr)->next)

#define for_each_area(hdr, head)     \
    for ((hdr) = (head); (hdr) != NULL; (hdr) = (hdr)->next)



#endif //QUARKOS_MM_VMALLOC_H
