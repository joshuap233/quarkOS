//
// Created by pjs on 2021/4/8.
//

#ifndef QUARKOS_MM_MM_H
#define QUARKOS_MM_MM_H

#define PAGE_SIZE           (4 * K)
#define PHYMM               (4 * G)              //最大物理内存大小
#define VIRMM               (4 * G)              //虚拟内存大小
#define ALIGN_MASK          ((uint32_t)(PAGE_SIZE - 1)) //页对齐掩码
#define PAGE_ADDR(addr)     ((addr) & (~MASK_U32(12)))


#define K_START ((pointer_t) _startKernel)
#define K_END   ((pointer_t) _endKernel)
#define K_SIZE  (K_END - K_START)


// 内核最大地址+1, 不要修改成 *_end,
extern char _endKernel[], _startKernel[];

#define SIZE_ALIGN(s)       (((s) & ALIGN_MASK) ?(((s)&(~ALIGN_MASK))+PAGE_SIZE):(s))


#endif //QUARKOS_MM_MM_H
