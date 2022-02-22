//
// Created by pjs on 2021/4/8.
//

#ifndef QUARKOS_MM_MM_H
#define QUARKOS_MM_MM_H

#include <highmem.h>

#define PAGE_SIZE           (4 * K)
#define PAGE_MASK           ((uint32_t)(PAGE_SIZE - 1)) //页对齐掩码
#define PAGE_CEIL(addr)     ((addr) & (~MASK_U32(12)))


/* -------- 虚拟内存--------- */
#define PDE_INDEX(addr)     ((addr)>>22)                //页目录索引
#define PTE_INDEX(addr)     (((addr) >> 12) & MASK_U32(10))

#define PTE_SIZE            sizeof(pte_t)                   //页表项大小
#define PDE_SIZE            sizeof(pde_t)
#define N_PTE               (PAGE_SIZE/PTE_SIZE)  //每个页面的表项数
#define N_PDE               (PAGE_SIZE/PDE_SIZE)


#define VM_A_MASK           (0b1<<5) //访问位掩码
#define VM_D_MASK           (0b1<<6) //脏位掩码
#define VM_PRES             0b1    // 在物理内存中
#define VM_NPRES            0b0    // 不在物理内存中
#define VM_KR               0b000  // 特权可读页
#define VM_KRW              0b010  // 特权可读写页
#define VM_UR               0b100  // 用户可读页
#define VM_URW              0b110

#define VM_RW_BIT           1
// 9-11 为被 cpu 忽略
#define VM_IGNORE_BIT1         0x200
#define VM_IGNORE_BIT2         0x400
#define VM_IGNORE_BIT3         0x800

#define MEM_ALIGN(s, align) (((s)+(align)-1)&(~((align)-1)))

#define PAGE_FLOOR(s)       (((s)+PAGE_MASK)&(~PAGE_MASK))

#endif //QUARKOS_MM_MM_H
