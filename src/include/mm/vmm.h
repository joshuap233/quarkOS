//
// Created by pjs on 2021/2/1.
//
// 页表使用递归映射
#ifndef QUARKOS_VMM_H
#define QUARKOS_VMM_H

#include "types.h"


#define PAGE_SIZE           (4 * K)
#define PHYMM               (4 * G)              //最大物理内存大小
#define VIRMM               (4 * G)              //虚拟内存大小
#define N_PPAGE             (PHYMM / PAGE_SIZE)  //页面数
#define N_VPAGE             (VIRMM / PAGE_SIZE) //虚拟页面数
#define ALIGN_MASK          ((uint32_t)(PAGE_SIZE - 1)) //页对齐掩码

#define PAGE_ADDR(addr)     ((addr) & (~MASK_U32(12)))
#define PDE_INDEX(addr)     ((addr)>>22)                //页目录索引
#define PTE_INDEX(addr)     (((addr) >> 12) & MASK_U32(10))

#define PTE_SIZE            sizeof(pte_t)                   //页表项大小
#define PDE_SIZE            sizeof(pde_t)
#define N_PTE              (PAGE_SIZE/PTE_SIZE)  //每个页面的表项数
#define N_PDE              (PAGE_SIZE/PDE_SIZE)
#define PT_SIZE             N_PDE * N_PTE * PTE_SIZE // 页表项总大小
#define SIZE_ALIGN(s)       (((s) & ALIGN_MASK) ?(((s)&(~ALIGN_MASK))+PAGE_SIZE):(s))

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


#define K_START ((pointer_t) _startKernel)
#define K_END   ((pointer_t) _endKernel)
#define K_SIZE  (K_END - K_START)

// 内核最大地址+1, 不要修改成 *_end,
extern char _endKernel[], _startKernel[];

typedef uint32_t entry;
typedef uint32_t cr3_t;
typedef entry pde_t;
typedef entry pte_t;

typedef struct table {
    entry entry[PAGE_ENTRY_NUM];
} table_t;

typedef table_t pdr_t;
typedef table_t ptb_t;


void vmm_mapv(pointer_t va, uint32_t size, uint32_t flags);

void vmm_unmap(void *va, uint32_t size);

void vmm_map(pointer_t va, pointer_t pa, uint32_t size, uint32_t flags);

//页错误错误码
typedef struct pf_error_code {
    uint16_t p: 1;   //置 0 则异常由页不存在引起,否则由特权级保护引起
    uint16_t w: 1;   //置 0 则访问是读取
    uint16_t u: 1;   //置 0 则特权模式下发生的异常
    uint16_t r: 1;
    uint16_t i: 1;
    uint16_t pk: 1;
    uint16_t zero: 10;
    uint16_t sgx: 1;
    uint16_t zero1: 15;
}PACKED pf_error_code_t;


// ============测试===================

void test_vmm_map();

void test_vmm_map2();

void test_vmm_mapv();

#endif //QUARKOS_VMM_H
