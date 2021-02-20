//
// Created by pjs on 2021/2/19.
//
#include "kthread.h"
#include "heap.h"
#include "qlib.h"
#include "qmath.h"


//用于管理空闲 tid
static struct kthread_map {
    uint8_t map[DIV_CEIL(KTHREAD_NUM, 8)];
    uint32_t len;
} tid_map = {
        .len = DIV_CEIL(KTHREAD_NUM, 8),
        .map = {0},
};


//TODO: 修改 r_task, join_task, finish_task, tid_map 加锁

//需要等待的线程
tcb_t *join_task = NULL;
//已执行完成等待销毁的线程
tcb_t *finish_task = NULL;
//正在运行或已经运行完成的线程
tcb_t *runnable_task = NULL;

static kthread_t alloc_tid() {
    for (uint32_t index = 0; index < tid_map.len; ++index) {
        uint8_t value = tid_map.map[index];
        for (int bit = 0; bit < 8; ++bit, value >>= bit) {
            if (!(value & 0b1)) {
                set_bit(&tid_map.map[index], bit);
                return index * 8 + bit;
            }
        }
    }
    // 0 号线程始终被内核占用
    return 0;
}

static void free_tid(kthread_t tid) {
    clear_bit(&tid_map.map[tid / 8], tid % 8);
}


// 回收 finish_task 链表中的线程
static void thread_recycle() {
    while (finish_task != NULL) {
        tcb_t *temp = finish_task;
        finish_task = finish_task->next;
        free_tid(temp->tid);
        freeK(temp->stack);
        freeK(temp);
    }
}

//初始化内核主线程
void sched_init() {
    register uint32_t esp asm("esp");
    runnable_task = mallocK(sizeof(tcb_t));
    runnable_task->prev = runnable_task;
    runnable_task->next = runnable_task;
    runnable_task->context.esp = esp;
    runnable_task->tid = alloc_tid();
    runnable_task->state = TASK_RUNNING_OR_RUNNABLE;
}


//void kthread_exit(void *ret)
static void kthread_exit_(tcb_t *tcb) {
//    disable_interrupt();
    runnable_task = runnable_task->next;
    finish_task->next = tcb;
    if (join_task == NULL) {
        thread_recycle();
    }
//    enable_paging();
//    schedule();
}

static void kthread_worker(void *(worker)(void *), void *args, tcb_t *tcb) {
    //传入 tcb 而不是使用 r_task.cur_thread,
    //防止 调用 kthread_exit_ 前,当前线程已经被切换
    worker(args);
    kthread_exit_(tcb);
}


// 成功返回 0,否则返回错误码
int kthread_create(void *(worker)(void *), void *args) {
    tcb_t *thread = mallocK(sizeof(tcb_t));
    thread->state = TASK_RUNNING_OR_RUNNABLE;
    thread->stack = mallocK(KTHREAD_STACK_SIZE);

    pointer_t *stack = thread->stack + KTHREAD_STACK_SIZE - sizeof(pointer_t);
    *stack = (pointer_t) thread;
    stack--;
    *stack = (pointer_t) args;
    stack--;
    *stack = (pointer_t) worker;    //kthread_worker 函数参数
    stack--;
    *stack = (pointer_t) schedule;  //worker 函数返回地址
    stack--;
    *stack = (pointer_t) kthread_worker;  //switch_to ret 指令会 pop worker 开始地址到 eip

    thread->context.esp = (pointer_t) stack;
    thread->context.eflags = 0x200; //开启中断
    thread->tid = alloc_tid();
    assertk(thread->tid != 0);

    //将线程插入到当前线程前
    thread->next = runnable_task;
    thread->prev = runnable_task->prev;
    runnable_task->prev->next = thread;
    runnable_task->prev = thread;
    return 0;
}


void schedule() {
    if (runnable_task != NULL && runnable_task->next != runnable_task) {
        //TODO: 如果添加线程切换函数,则需要在切换时禁止中断
        runnable_task = runnable_task->next;
        switch_to(&runnable_task->prev->context, &runnable_task->context);
    }
}

// 成功返回0,否则返回错误码
int kthread_join(kthread_t tid, void **value_ptr) {
    return 0;
}