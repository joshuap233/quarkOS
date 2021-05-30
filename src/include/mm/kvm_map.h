//
// Created by pjs on 2021/2/1.
//
// 页表使用递归映射
#ifndef QUARKOS_MM_VMM_H
#define QUARKOS_MM_VMM_H

#include "types.h"
#include "mm/mm.h"


#define N_PPAGE             (PHYMM / PAGE_SIZE)  //页面数
#define N_VPAGE             (VIRMM / PAGE_SIZE) //虚拟页面数

#define PDE_INDEX(addr)     ((addr)>>22)                //页目录索引
#define PTE_INDEX(addr)     (((addr) >> 12) & MASK_U32(10))

#define PTE_SIZE            sizeof(pte_t)                   //页表项大小
#define PDE_SIZE            sizeof(pde_t)
#define N_PTE              (PAGE_SIZE/PTE_SIZE)  //每个页面的表项数
#define N_PDE              (PAGE_SIZE/PDE_SIZE)
#define PT_SIZE             N_PDE * N_PTE * PTE_SIZE // 页表项总大小

#define CR3_CTRL 0         //不使用 write-through,且页目录允许缓存
#define PAGE_ENTRY_NUM     (PAGE_SIZE / sizeof(entry))

#define VM_A_MASK          (0b1<<5) //访问位掩码
#define VM_D_MASK          (0b1<<6) //脏位掩码
#define VM_PRES          0b1    // 在物理内存中
#define VM_NPRES         0b0    // 不在物理内存中
#define VM_KR            0b100  // 特权可读页
#define VM_KW            0b110  // 特权可读写页
#define VM_UR            0b000  // 用户可读页
#define VM_UW            0b010


typedef uint32_t entry;
typedef uint32_t cr3_t;
typedef entry pde_t;
typedef entry pte_t;

typedef struct table {
    entry entry[PAGE_ENTRY_NUM];
} table_t;

//#define pdr_t  entry[PAGE_ENTRY_NUM];

typedef table_t pdr_t;
typedef table_t ptb_t;


void kvm_mapv(ptr_t va, uint32_t size, uint32_t flags);

void kvm_unmap(void *va, uint32_t size);

void kvm_mapd(ptr_t va, ptr_t pa, uint32_t size, uint32_t flags);

void kvm_mapPage(ptr_t va, ptr_t pa, uint32_t flags);

ptr_t kvm_vm2pm(ptr_t va);

void kvm_unmapPage(ptr_t va);

typedef ptr_t pointer_t;
#endif //QUARKOS_MM_VMM_H
