//
// Created by pjs on 2021/5/17.
//

#ifndef QUARKOS_SCHED_TCB_H
#define QUARKOS_SCHED_TCB_H

#include "types.h"
#include "mm/mm.h"
#include "fs/vfs.h"
#include "lib/qlib.h"

/*
 * 线程切换时需要保存的上下文
 * 根据x86 systemV ABI eax, ecx, edx 是临时寄存器,使用(schedule)函数手动切换时无需保存,
 * 使用中断切换时, __attribute__((interrupt)) 会保存 eax ecx, edx
 * 段选择子固定, 不使用 ldt, 都无需保存
 * 内核线程不需要保存 cr3, call 指令保存 eip
 */
typedef struct context {
    uint32_t esp;
    uint32_t eflags;
    uint32_t ebx;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
} context_t;


typedef uint32_t kthread_t; //线程id

typedef enum kthread_state {
    TASK_RUNNING = 0,     // 线程可运行(或正在运行)
    TASK_SLEEPING,        // 线程睡眠
    TASK_ZOMBIE           // 线程运行完成等待回收
} kthread_state;

typedef struct tcb {
#define KTHREAD_NAME_LEN   16
#define KTHREAD_STACK_SIZE PAGE_SIZE
#define KTHREAD_NUM        1024
#define MAX_PRIORITY       3
    list_head_t run_list;    //运行队列
#ifdef DEBUG
#define THREAD_MAGIC       0x18ee7305
    u32_t magic;
    u32_t spin_cnt;          // 自旋锁自旋次数
    u64_t last_run_time;     // 上次运行时间
#endif // DEBUG
    kthread_t tid;
    kthread_state state;
    u16_t priority;
    u16_t timer_slice;        // 时间片
    char name[KTHREAD_NAME_LEN];
    void *stack;              // 指向栈首地址,用于回收
    inode_t *cwd;
    context_t context;        // 上下文信息
} tcb_t;


// i r esp / ~(uint32_t)(4096 - 1)
INLINE tcb_t *cur_tcb() {
    tcb_t *tcb;
    asm("andl %%esp,%0; ":"=r" (tcb): "0" (~PAGE_MASK));
    return tcb;
}

#ifdef DEBUG

INLINE tcb_t *_cur_tcb() {
    tcb_t *tcb = cur_tcb();
    assertk(tcb->magic == THREAD_MAGIC);
    return tcb;
}

#define CUR_TCB  _cur_tcb()

#else

#define CUR_TCB  cur_tcb()

#endif //DEBUG


#define CUR_HEAD (CUR_TCB->run_list)

#ifdef DEBUG

INLINE tcb_t *tcb_entry(list_head_t *ptr) {
    tcb_t *thread = list_entry(ptr, tcb_t, run_list);
    assertk(((ptr_t) thread & PAGE_MASK) == 0);
    assertk(thread->magic == THREAD_MAGIC);
    return thread;
}

#else

#define tcb_entry(ptr) list_entry(ptr,tcb_t,run_list)

#endif // DEBUG

#endif //QUARKOS_SCHED_TCB_H
