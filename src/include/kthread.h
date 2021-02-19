//
// Created by pjs on 2021/2/18.
//
// 内核线程

#ifndef QUARKOS_KTHREAD_H
#define QUARKOS_KTHREAD_H

#include "types.h"

typedef struct context {
    //线程切换时需要保存的上下文
    //根据x86 systemV ABI eax, ecx, edx 是临时寄存器,
    //段选择子固定, 不使用 ldt, 都无需保存
    //内核线程不需要保存 cr3, call/中断切换线程时,会保存 eip
    uint32_t esp;
    uint32_t eflags;
    uint32_t ebx;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
} context_t;

typedef enum kthread_state {
    TASK_RUNNING_OR_RUNNABLE = 0,
    TASK_SLEEPING = 1,
    TASK_ZOMBIE = 2,
    TASK_STOPPED = 3
} kthread_state_t;

typedef uint32_t kthread_t; //线程id

typedef struct tcb {
#define KTHREAD_NAME_LEN   16
#define KTHREAD_STACK_SIZE 4096
    kthread_t tid;
    kthread_state_t state;
//    char name[KTHREAD_NAME_LEN];
    context_t context; //上下文信息
    struct tcb *next;
} tcb_t;

//可运行或正在运行线程
typedef struct running_task {
    tcb_t *cur_thread;
    tcb_t *prev_thread;
} running_task_t;

void sched_init();

int kthread_create(void *(worker)(void *args), void *args);

void schedule();

int kthread_join(kthread_t tid, void **value_ptr);

void kthread_exit();

extern void switch_to(context_t *cur_context, context_t *next_context);

#endif //QUARKOS_KTHREAD_H
