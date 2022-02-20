//
// Created by pjs on 2021/5/31.
//

#ifndef QUARKOS_HIGHMEM_H
#define QUARKOS_HIGHMEM_H

#define KERNEL_START 0xc0000000
#define STACK_SIZE   PAGE_SIZE

#define K_END   ((ptr_t) _endKernel-KERNEL_START)

// 内核最大地址+1, 不要修改成 *_end,
extern char _endKernel[], _startKernel[];
extern char _rodataStart[], _dataStart[];

#endif //QUARKOS_HIGHMEM_H
