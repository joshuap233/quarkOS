//
// Created by pjs on 2021/2/18.
//
// 内核线程

#ifndef QUARKOS_KTHREAD_H
#define QUARKOS_KTHREAD_H

#include "types.h"

struct context {
    //根据x86 systemV ABI eax, ecx, edx 是临时寄存器,
    //段选择子固定, 不使用 ldt, 都无需保存

    uint32_t cr3;
    uint32_t eip;
    uint32_t esp;
    uint32_t eflags;
    uint32_t ebx;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
};

typedef enum process_state {
    TASK_RUNNING_OR_RUNNABLE = 0,
    TASK_SLEEPING = 1,
    TASK_ZOMBIE = 2,
    TASK_STOPPED = 3
}process_state_t;

typedef struct pcb {
#define TASK_NAME_LEN 16
    uint32_t pid;
    process_state_t state;
    char name[TASK_NAME_LEN];
    struct context context;
    struct pcb *next;
} pcb_t;

void task_init();

#endif //QUARKOS_KTHREAD_H
