//
// Created by pjs on 2021/2/1.
//
// 页表使用递归映射
#ifndef QUARKOS_MM_KVM_H
#define QUARKOS_MM_KVM_H

#include <types.h>
#include <mm/mm.h>
#include <mm/page.h>

#define N_PPAGE             (PHYMM / PAGE_SIZE)  //页面数
#define N_VPAGE             (VIRMM / PAGE_SIZE) //虚拟页面数

#define PDE_INDEX(addr)     ((addr)>>22)                //页目录索引
#define PTE_INDEX(addr)     (((addr) >> 12) & MASK_U32(10))

#define PTE_SIZE            sizeof(pte_t)                   //页表项大小
#define PDE_SIZE            sizeof(pde_t)
#define N_PTE               (PAGE_SIZE/PTE_SIZE)  //每个页面的表项数
#define N_PDE               (PAGE_SIZE/PDE_SIZE)
#define PT_SIZE             N_PDE * N_PTE * PTE_SIZE // 页表项总大小

#define CR3_CTRL 0          //不使用 write-through,且页目录允许缓存

#define VM_A_MASK           (0b1<<5) //访问位掩码
#define VM_D_MASK           (0b1<<6) //脏位掩码
#define VM_PRES             0b1    // 在物理内存中
#define VM_NPRES            0b0    // 不在物理内存中
#define VM_KR               0b000  // 特权可读页
#define VM_KW               0b010  // 特权可读写页
#define VM_UR               0b100  // 用户可读页
#define VM_UW               0b110

#define VM_RW_BIT           1
// 9-11 为被 cpu 忽略
#define VM_IGNORE_BIT1         0x200
#define VM_IGNORE_BIT2         0x400
#define VM_IGNORE_BIT3         0x800

typedef uint32_t cr3_t;
typedef uint32_t pde_t;
typedef uint32_t pte_t;


void kvm_unmap(struct page *page);

void kvm_map(struct page *page, uint32_t flags);

ptr_t kvm_vm2pm(ptr_t va);

ptr_t kvm_pm2vm(ptr_t pa);

void kvm_copy(pde_t *pgdir);

struct page *kvm_vm2page(ptr_t va);

struct page *va_get_page(ptr_t addr);

void switch_kvm();

void switch_uvm(pde_t *pgdir);

#endif //QUARKOS_MM_KVM_H