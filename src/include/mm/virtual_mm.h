//
// Created by pjs on 2021/2/1.
//

#ifndef QUARKOS_VIRTUAL_MM_H
#define QUARKOS_VIRTUAL_MM_H

#include "types.h"
#include "mm/mm.h"
#include "free_list.h"


#define VM_A_MASK          (0b1<<5) //访问位掩码
#define VM_D_MASK          (0b1<<6) //脏位掩码
#define VM_PRES          0b1    // 在物理内存中
#define VM_NPRES         0b0    // 不在物理内存中
#define VM_KR            0b100  // 特权可读页
#define VM_KW            0b110  // 特权可读写页
#define VM_UR            0b000  // 用户可读页
#define VM_UW            0b010

//使用页目录递归映射,页表虚拟地址为 PTD_VA ~ PTD_VA + PT_SIZE
//分配新页表时不需要再额外映射页表地址
#define PTE_VA          0xffc00000 //PDE 虚拟地址
//返回页目录索引为index的页表项首地址
#define PTE_ADDR(pde_index) (PTE_VA + (pde_index)*PAGE_SIZE)

void vmm_init();

void vmm_mapv(pointer_t va, uint32_t size, uint32_t flags);

void vmm_unmap(void *va, uint32_t size);

void vmm_map(pointer_t va, pointer_t pa, uint32_t size, uint32_t flags);

//只分配虚拟地址
#define vmm_alloc_vm list_split_ff

extern void cr3_set(pointer_t);

#endif //QUARKOS_VIRTUAL_MM_H
