//
// Created by pjs on 2021/6/3.
//

#include <sched/fork.h>
#include <lib/list.h>
#include <fs/vfs.h>

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


typedef u32_t pid_t;

typedef enum task_state {
    TASK_RUNNING = 0,     // 线程可运行(或正在运行)
    TASK_SLEEPING,        // 线程睡眠
    TASK_ZOMBIE           // 线程运行完成等待回收
} task_state;


struct task_struct {
#define TASK_NAME_LEN   16

    list_head_t run_list;     //运行队列

    pid_t tid;
    task_state state;
    u16_t priority;           // 优先级
    u16_t timer_slice;        // 时间片
    char name[TASK_NAME_LEN];
    void *stack;              // 内核栈地址
    ptr_t *pgdir;             // 页目录物理地址
    struct mm_struct *mm;     // 分配的虚拟内存,内核线程共用一个

    inode_t *cwd;
    context_t context;        // 上下文信息
};

// 位于内核栈底部
struct thread_info {
    struct task_struct *task;
};