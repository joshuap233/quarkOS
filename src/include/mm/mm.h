//
// Created by pjs on 2021/4/8.
//

#ifndef QUARKOS_MM_MM_H
#define QUARKOS_MM_MM_H

#include <highmem.h>

#define PAGE_SIZE           (4 * K)
#define PHYMM               (4 * G)              //最大物理内存大小
#define VIRMM               (4 * G)              //虚拟内存大小
#define PAGE_MASK           ((uint32_t)(PAGE_SIZE - 1)) //页对齐掩码
#define PAGE_ADDR(addr)     ((addr) & (~MASK_U32(12)))




#define MEM_ALIGN(s, align) (((s)+(align)-1)&(~((align)-1)))

#define PAGE_ALIGN(s)       (((s)+PAGE_MASK)&(~PAGE_MASK))

#endif //QUARKOS_MM_MM_H
