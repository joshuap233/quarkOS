//
// Created by pjs on 2021/5/7.
//
#include "mm/vm_area.h"
#include "mm/mm.h"
#include "mm/vmm.h"
#include "lib/qlib.h"
#include "mm/kmalloc.h"
#include "mm/block_alloc.h"

#define KERNEL_SECTION_NUM 3


// 内核预留区域
struct vm_area kvmArea;
struct area kArea[KERNEL_SECTION_NUM + 1];
struct vm_area vm_area_head;

void vm_area_init() {
    // 内核区域与虚拟地址直接线性映射,因此物理地址就是内核的虚拟地址
    // TODO: 如果内核映射到 3-4G 则需要改修改下面的代码
    vm_area_head.area = NULL;
    vm_area_head.next = &kvmArea;

    kvmArea.type = KERNEL_AREA;
    kvmArea.area = &kArea[0];
    kvmArea.next = NULL;

    // 低于 1M 的内存区域
    kArea[0].addr = 0;
    kArea[0].size = (ptr_t) _startKernel;
    kArea[0].flag = VM_PRES | VM_KW;
    kArea[0].next = &kArea[1];


    // text 段
    kArea[1].addr = (ptr_t) _startKernel;
    kArea[1].size = (ptr_t) _rodataStart - (ptr_t) _startKernel;
    kArea[1].flag = VM_PRES | VM_KR;
    kArea[1].next = &kArea[2];

    // rodada段
    kArea[2].addr = (ptr_t) _rodataStart;
    kArea[2].size = (ptr_t) _dataStart - (ptr_t) _rodataStart;
    kArea[2].flag = VM_PRES | VM_KR;
    kArea[2].next = &kArea[3];

    // data 段, bss 段 与初始化内核分配的内存
    kArea[3].addr = (ptr_t) _dataStart;
    kArea[3].size = (ptr_t) _endKernel - (ptr_t) _dataStart;
    kArea[3].flag = VM_PRES | VM_KW;
    kArea[3].next = NULL;
}


void vm_area_add(ptr_t addr, u32_t size, u16_t flag, enum AREA_TYPE type) {
    struct vm_area *hdr;
    for_each_vm_area(hdr) {
        if (hdr->type == type) break;
    }

    if (hdr == NULL) {
        hdr = kmalloc(sizeof(struct vm_area));
        hdr->type = type;
        hdr->next = vm_area_head.next;
        vm_area_head.next = hdr;
    }

    struct area *area = kmalloc(sizeof(struct area));
    area->addr = addr;
    area->size = size;
    area->flag = flag;
    area->next = hdr->area->next;
    hdr->area = area;
    assertk(0);
}

void vm_area_del(ptr_t addr, u32_t size, enum AREA_TYPE type) {
    assertk(type != KERNEL_AREA);

}


// 扩展地址值最大的区域
void vm_area_expand(u32_t end_addr, enum AREA_TYPE type) {
    struct vm_area *hdr;
    for_each_vm_area(hdr) {
        if (hdr->type == type) {
            struct area *area, *temp = hdr->area;
            for_each_area(area, hdr->area->next) {
                if (area->addr > temp->addr)
                    temp = area;
            }

            assertk(end_addr > temp->addr);
            temp->size += end_addr - temp->addr;
            return;
        }
    }
    assertk(0);
}