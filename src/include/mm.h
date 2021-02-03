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
#define PHYMM               (4 * G)
#define N_PAGE              (PHYMM / PAGE_SIZE) //页面数
#define ALIGN_MASK          ((uint32_t)(PAGE_SIZE - 1)) //页对齐掩码
#define PAGE_SHIT           12
#define MM_NULL             ((void*)(PHYMM - 1) )  //空页表指针
#define PTE_ENTRY_SIZE      4                   //页表项大小
#define PDE_ENTRY_SIZE      4
#define N_PTE              (PAGE_SIZE/PTE_ENTRY_SIZE)  //每个页面的表项数
#define N_PDE              (PAGE_SIZE/PDE_ENTRY_SIZE)

// 使 a 参数页对齐
#define PAGE_ALIGN(a)       ((a) & ALIGN_MASK) \
                                ?((a)&(~ALIGN_MASK)):(a)

#define A_MASK          (0b1<<5) //访问位掩码
#define D_MASK          (0b1<<6) //脏位掩码
#define MM_PRES          0b1    // 在物理内存中
#define MM_NPRES         0b0    // 不在物理内存中
#define MM_KR            0b100  // 特权可读页
#define MM_KW            0b110  // 特权可读写页
#define MM_UR            0b000  // 用户可读页
#define MM_UW            0b010

#define CR3_CTRL 0       //不使用 write-through,且页目录允许缓存

//采用 4K 页(第 7 位置0)


typedef uint32_t pde_t;
typedef uint32_t cr3_t;
typedef uint32_t pte_t;


void mm_init();

// 内核最大地址+1, 不要修改成 *_end,
extern char _endKernel[], _startKernel[];

#define K_START ((pointer_t) _startKernel)
#define K_END   ((pointer_t) _endKernel)
#define K_SIZE  (K_END - K_START)

#endif //QUARKOS_MM_H
