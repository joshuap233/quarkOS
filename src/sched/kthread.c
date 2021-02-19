//
// Created by pjs on 2021/2/19.
//

#include "kthread.h"
#include "heap.h"
#include "qlib.h"

running_task_t r_task = {
        .cur_thread = NULL,
        .prev_thread = NULL
};

//需要等待的线程
tcb_t *join_task = NULL;
//已执行完成等待销毁的线程
tcb_t *finish_task = NULL;

//初始化内核主线程
void sched_init() {
    register uint32_t esp asm("esp");
    r_task.cur_thread = mallocK(sizeof(tcb_t));
    r_task.prev_thread = r_task.cur_thread;
    r_task.cur_thread->next = r_task.cur_thread;
    r_task.cur_thread->context.esp = esp;
    r_task.cur_thread->tid = 0;
    r_task.cur_thread->state = TASK_RUNNING_OR_RUNNABLE;
}


//void kthread_exit(void *ret) {
void kthread_exit() {
    printfk("exit\n");
}

// 成功返回 0,否则返回错误码
int kthread_create(void *(worker)(void *args), void *args) {
    tcb_t *thread = mallocK(sizeof(tcb_t));
    thread->state = TASK_RUNNING_OR_RUNNABLE;

    pointer_t *stack = mallocK(KTHREAD_STACK_SIZE) + KTHREAD_STACK_SIZE - sizeof(pointer_t);
    *stack = (pointer_t) args;          //worker 函数参数
    stack--;
    *stack = (pointer_t) kthread_exit;  //worker 函数返回地址
    stack--;
    *stack = (pointer_t) worker;  //switch_to ret 指令会 pop worker 开始地址到 eip

    thread->context.esp = (pointer_t) stack;
    thread->context.eflags = 0x200; //开启中断

    //将线程插入到当前线程前
    thread->next = r_task.cur_thread;
    r_task.prev_thread->next = thread;
    r_task.prev_thread = thread;
    return 0;
}


void schedule() {
    if (r_task.cur_thread != NULL && r_task.cur_thread->next != r_task.cur_thread) {
        //TODO: 如果添加线程切换函数,则需要在切换时禁止中断
        r_task.prev_thread = r_task.cur_thread;
        r_task.cur_thread = r_task.cur_thread->next;
        switch_to(&r_task.prev_thread->context, &r_task.cur_thread->context);
    }
}

// 成功返回0,否则返回错误码
int kthread_join(kthread_t tid, void **value_ptr) {

    return 0;
}