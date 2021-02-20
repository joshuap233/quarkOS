//
// Created by pjs on 2021/2/19.
//
#include "kthread.h"
#include "heap.h"
#include "qlib.h"
#include "qmath.h"

//使用这种方式会导致中断丢失,即无论调用 kLock 前是否开启中断
//调用 kRelease 后都会开启中断
static inline void k_lock() {
    disable_interrupt();
}

static inline void k_lock_release() {
    enable_interrupt();
}

_Noreturn static inline void Idle(){
    while (1){
        halt();
    }
}

//用于管理空闲 tid
static struct kthread_map {
    uint8_t map[DIV_CEIL(KTHREAD_NUM, 8)];
    uint32_t len;
} tid_map = {
        .len = DIV_CEIL(KTHREAD_NUM, 8),
        .map = {0},
};


//需要等待的线程
tcb_t *join_task = NULL;
//已执行完成等待销毁的线程
tcb_t *finish_task = NULL;
//正在运行或已经运行完成的线程
tcb_t *cur_task = NULL;


static kthread_t alloc_tid() {
    for (uint32_t index = 0; index < tid_map.len; ++index) {
        k_lock();
        uint8_t value = tid_map.map[index];
        for (int bit = 0; bit < 8; ++bit, value >>= bit) {
            if (!(value & 0b1)) {
                set_bit(&tid_map.map[index], bit);
                return index * 8 + bit;
            }
        }
        k_lock_release();
    }
    // 0 号线程始终被内核占用
    return 0;
}

static void free_tid(kthread_t tid) {
    k_lock();
    clear_bit(&tid_map.map[tid / 8], tid % 8);
    k_lock_release();
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
    cur_task = mallocK(sizeof(tcb_t));
    cur_task->prev = cur_task;
    cur_task->next = cur_task;
    cur_task->context.esp = esp;
    cur_task->tid = alloc_tid();
    cur_task->state = TASK_RUNNING_OR_RUNNABLE;
}


//void kthread_exit(void *ret)
static void kthread_exit_(tcb_t *tcb) {
    k_lock();
    finish_task->next = tcb;
    tcb->prev = tcb->next;
    if (join_task == NULL) {
        thread_recycle();
    }
    k_lock_release();
    Idle();
}

static void kthread_worker(void *(worker)(void *), void *args, tcb_t *tcb) {
    //传入 tcb 而不是使用 r_task.cur_thread,
    //防止 调用 kthread_exit_ 前,当前线程已经被切换
    k_lock_release();
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
    k_lock();
    thread->next = cur_task;
    thread->prev = cur_task->prev;
    cur_task->prev->next = thread;
    cur_task->prev = thread;
    k_lock_release();
    return 0;
}


void schedule() {
    k_lock();
    if (cur_task != NULL && cur_task->next != cur_task) {
        cur_task = cur_task->next;
        switch_to(&cur_task->prev->context, &cur_task->context);
    }
}

// 成功返回0,否则返回错误码
int kthread_join(kthread_t tid, void **value_ptr) {
    return 0;
}