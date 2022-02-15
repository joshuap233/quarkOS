//
// Created by pjs on 2022/2/10.
//

#ifndef QUARKOS_KERNEL_CPU_H
#define QUARKOS_KERNEL_CPU_H

#include <limit.h>
#include <lib/list.h>

struct cpu {
    u8_t apic_id;          // lapic id
    bool ir_enable;        // 是否开启中断
    struct spinlock *lock; // 当前 cpu 线程持有的自旋锁
    list_head_t *idle;
};

extern struct cpu cpus[N_CPU];

#endif //QUARKOS_KERNEL_CPU_H
