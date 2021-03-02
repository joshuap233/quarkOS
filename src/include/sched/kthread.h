//
// Created by pjs on 2021/2/18.
//
// 内核线程

#ifndef QUARKOS_KTHREAD_H
#define QUARKOS_KTHREAD_H

#include "types.h"
#include "klib/list.h"

typedef struct context {
    //线程切换时需要保存的上下文
    //根据x86 systemV ABI eax, ecx, edx 是临时寄存器,使用函数手动切换时无需保存,
    //使用中断切换时, __attribute__((interrupt)) 会保存 eax ecx, edx
    //段选择子固定, 不使用 ldt, 都无需保存
    //内核线程不需要保存 cr3, call 指令保存 eip
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
    TASK_ZOMBIE = 2, //线程终止等待回收
} kthread_state_t;

typedef uint16_t kthread_t; //线程id

typedef struct tcb {
#define KTHREAD_NAME_LEN   16
#define KTHREAD_STACK_SIZE 4096
#define KTHREAD_NUM        65536
    struct tcb *next, *prev;
    kthread_t tid;
    kthread_state_t state;
    void *stack;       //指向栈首地址,用于回收
    context_t context; //上下文信息
} tcb_t;


void sched_init();

int kthread_create(void *(worker)(void *args), void *args);

void schedule();

int kthread_join(kthread_t tid, void **value_ptr);

void kthread_exit();

void block_thread();

void unblock_thread(tcb_t *thread);

extern void switch_to(context_t *cur_context, context_t *next_context);

extern tcb_t *runnable_task;
#define cur_task runnable_task

#endif //QUARKOS_KTHREAD_H
