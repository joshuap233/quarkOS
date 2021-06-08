//
// Created by pjs on 2021/6/5.
//

#ifndef QUARKOS_TASK_TASK_H
#define QUARKOS_TASK_TASK_H

#include <types.h>
#include <lib/list.h>
#include <fs/vfs.h>
#include <mm/mm.h>
#include <mm/vmalloc.h>

#define TASK_NAME_LEN           16
#define TASK_NUM                1024
#define TASK_MAX_PRIORITY       3
#define TASK_MAGIC       0x18ee7305

/*
 * 线程切换时需要保存的上下文
 * 根据x86 systemV ABI eax, ecx, edx 是临时寄存器,使用(schedule)函数手动切换时无需保存,
 * 使用中断切换时, __attribute__((interrupt)) 会保存 eax ecx, edx
 * 段选择子固定, 不使用 ldt, 都无需保存
 *  eip 不需要手动保存, call 与 ret 会自动保存与还原
 *  esp 地址即为 context 地址
 */
typedef struct context {
    u32_t eflags;
    u32_t ebx;
    u32_t ebp;
    u32_t esi;
    u32_t edi;
    u32_t eip;
} context_t;

// 系统调用上下文
typedef struct sys_context {
    u32_t ds;
    u32_t es;
    u32_t fs;
    u32_t gs;
    u32_t eip;
    u32_t cs;
    u32_t eflags;
    u32_t esp;
    u32_t ss;
} sys_context_t;

typedef int32_t pid_t;
typedef int32_t kthread_t;

typedef enum task_state {
    TASK_RUNNING = 0,     // 线程可运行(或正在运行)
    TASK_SLEEPING,        // 线程睡眠
    TASK_ZOMBIE           // 线程运行完成等待回收
} task_state;


typedef struct task_struct {
    //TODO: 记录打开的文件用于回收
    list_head_t run_list;     //运行队列

    struct task_struct *parent; //父进程
    list_head_t child, sibling;

    pid_t pid;
    task_state state;

    u16_t priority;           // 优先级
    u16_t timer_slice;        // 时间片

    char name[TASK_NAME_LEN];

    void *stack;              // 内核栈底(低地址)

    struct mm_struct *mm;     // 分配的虚拟内存,内核线程共用一个

    inode_t *cwd;

    context_t *context;        // 上下文信息

    sys_context_t *sysContext;

    u32_t magic;
} tcb_t;

struct thread_info {
    struct task_struct *task;
};

// i r esp / ~(uint32_t)(4096 - 1)
INLINE tcb_t *cur_tcb() {
    tcb_t *tcb;
    asm("andl %%esp,%0; ":"=r" (tcb): "0" (~PAGE_MASK));
    return tcb;
}

#ifdef DEBUG

INLINE tcb_t *_cur_tcb() {
    tcb_t *tcb = cur_tcb();
    assertk(tcb->magic == TASK_MAGIC);
    return tcb;
}

INLINE tcb_t *tcb_entry(list_head_t *ptr) {
    tcb_t *task = list_entry(ptr, tcb_t, run_list);
    assertk(((ptr_t) task & PAGE_MASK) == 0);
    assertk(task->magic == TASK_MAGIC);
    return task;
}

#define CUR_TCB  _cur_tcb()

#else

#define CUR_TCB  cur_tcb()
#define tcb_entry(ptr) list_entry(ptr,tcb_t,run_list)

#endif //DEBUG

#define CUR_HEAD (CUR_TCB->run_list)


#endif //QUARKOS_TASK_TASK_H
