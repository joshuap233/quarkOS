//
// Created by pjs on 2021/2/19.
//
#include "sched/kthread.h"
#include "sched//klock.h"
#include "mm/heap.h"
#include "klib/qlib.h"
#include "klib/qmath.h"

_Noreturn static inline void Idle() {
    while (1) {
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


//cur_task 为循环链表, join_task 与 finish_task 不是
tcb_t *join_task = NULL; //需要等待的线程

tcb_t *finish_task = NULL; //已执行完成等待销毁的线程

tcb_t *cur_task = NULL; //正在运行或已经运行完成的线程



// dest->prev 插入src节点
static inline void list_inert_p(tcb_t *src, tcb_t *dest) {
    src->prev = dest->prev;
    src->next = dest;
    if (dest->prev != NULL) dest->prev->next = src;
    dest->prev = src;
}

// dest->next 插入src节点
static inline void list_inert_n(tcb_t *src, tcb_t *dest) {
    src->prev = dest;
    src->next = dest->next;
    if (dest->next != NULL) dest->next->prev = src;
    dest->next = src;
}

static inline void list_delete(tcb_t *node) {
    node->prev = node->next;
}


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
        k_unlock();
    }
    // 0 号线程始终被内核占用
    return 0;
}

static void free_tid(kthread_t tid) {
    k_lock();
    clear_bit(&tid_map.map[tid / 8], tid % 8);
    k_unlock();
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
void kthread_exit_(tcb_t *tcb) {
    k_lock();
    finish_task->next = tcb;
    tcb->prev = tcb->next;
    tcb->state = TASK_ZOMBIE;
    if (join_task == NULL) {
        thread_recycle();
    }
    k_unlock();
    Idle();
}

//传入 tcb 而不是使用 r_task.cur_thread, 防止 调用 kthread_exit_ 前,当前线程已经被切换
extern void kthread_worker(void *(worker)(void *), void *args, tcb_t *tcb);


// 成功返回 0,否则返回错误码
int kthread_create(void *(worker)(void *), void *args) {
    //tcb 结构放到栈底
    void *stack = mallocK(KTHREAD_STACK_SIZE);
    assertk(stack != NULL);
    tcb_t *thread = stack + KTHREAD_STACK_SIZE - sizeof(tcb_t);
    thread->state = TASK_RUNNING_OR_RUNNABLE;
    thread->stack = stack;

    pointer_t *esp = (void *) thread - sizeof(pointer_t) * 5;
    esp[4] = (pointer_t) thread;
    esp[3] = (pointer_t) args;
    esp[2] = (pointer_t) worker;    //kthread_worker 参数
    esp[1] = (pointer_t) NULL;      //kthread_worker 返回地址
    esp[0] = (pointer_t) kthread_worker;

    thread->context.esp = (pointer_t) esp;
    thread->context.eflags = get_eflags() | INTERRUPT_MASK; // 开启中断
    thread->tid = alloc_tid();
    assertk(thread->tid != 0);

    k_lock();
    list_inert_p(thread, cur_task);
    k_unlock();
    return 0;
}


void schedule() {
    k_lock();
    if (cur_task != NULL && cur_task->next != cur_task) {
        cur_task = cur_task->next;
        switch_to(&cur_task->prev->context, &cur_task->context);
    } else {
        k_unlock();
    }
}

// 成功返回0,否则返回错误码
int kthread_join(kthread_t tid, void **value_ptr) {
    k_lock();
    uint8_t error = 1;
    for (tcb_t *header = cur_task; header != NULL; header = header->next) {
        if (header->tid == tid) {
            list_delete(header);
            list_inert_n(header, join_task);
            break;
        }
    }
    k_unlock();
    return error;
}