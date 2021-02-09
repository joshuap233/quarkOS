//
// Created by pjs on 2021/2/1.
//

#ifndef QUARKOS_MM_H
#define QUARKOS_MM_H

#include "types.h"
#include "qlib.h"


#define K                   1024
#define M                   0x100000
#define G                   ((uint64_t)0x40000000)
#define PAGE_SIZE           (4 * K)
#define PHYMM               (4 * G)              //最大物理内存大小
#define VIRMM               (4 * G)              //虚拟内存大小
#define N_PPAGE             (PHYMM / PAGE_SIZE)  //页面数
#define N_VPAGE             (VIRMM / PAGE_SIZE) //虚拟页面数
#define ALIGN_MASK          ((uint32_t)(PAGE_SIZE - 1)) //页对齐掩码

#define PAGE_ADDR(addr)     ((addr) & (~MASK_U32(12)))
#define PDE_INDEX(addr)     ((addr)>>22)                //页目录索引
#define PTE_INDEX(addr)     (((addr) >> 12) & MASK_U32(10))

#define MM_NULL             ((void*)0)  //空页表指针, 存在位为 0
#define PTE_SIZE            sizeof(pte_t)                   //页表项大小
#define PDE_SIZE            sizeof(pde_t)
#define N_PTE              (PAGE_SIZE/PTE_SIZE)  //每个页面的表项数
#define N_PDE              (PAGE_SIZE/PDE_SIZE)
#define PT_SIZE             N_PDE * N_PTE * PTE_SIZE // 页表项总大小
// 使 a 页对齐(向上)
#define ADDR_ALIGN(a)      (((a) & ALIGN_MASK) ?((a)&(~ALIGN_MASK)):(a))

// 使 s 页对齐(向下)
#define SIZE_ALIGN(s)       (((s) & ALIGN_MASK) ?(((s)&(~ALIGN_MASK))+PAGE_SIZE):(s))


#define CR3_CTRL 0       //不使用 write-through,且页目录允许缓存

typedef uint32_t cr3_t;
typedef uint32_t pde_t;
typedef uint32_t pte_t;


void mm_init();

// 内核最大地址+1, 不要修改成 *_end,
extern char _endKernel[], _startKernel[];

#define K_START ((pointer_t) _startKernel)
#define K_END   ((pointer_t) _endKernel)
#define K_SIZE  (K_END - K_START)

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
}__attribute__((packed)) pf_error_code_t;

#endif //QUARKOS_MM_H
