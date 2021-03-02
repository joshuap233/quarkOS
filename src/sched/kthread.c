//
// Created by pjs on 2021/2/19.
//
#include "sched/kthread.h"
#include "sched//klock.h"
#include "mm/heap.h"
#include "klib/qlib.h"
#include "klib/qmath.h"
#include "sched/timer.h"



//static void _list_header_init(tcb_t *header) {
//    header->next = header;
//    header->prev = header;
//}
//
//__attribute__((always_inline))
//static inline void _list_add(tcb_t *new, tcb_t *prev, tcb_t *next) {
//    new->prev = prev;
//    new->next = next;
//    next->prev = new;
//    prev->next = new;
//}
//
//__attribute__((always_inline))
//static inline void _list_add_next(tcb_t *new, tcb_t *target) {
//    _list_add(new, target, target->next);
//}
//
//__attribute__((always_inline))
//static inline void _list_add_prev(tcb_t *new, tcb_t *target) {
//    _list_add(new, target->prev, target);
//}
//
//
//__attribute__((always_inline))
//static inline void _list_del(tcb_t *list) {
//    list->prev->next = list->next;
//    list->next->prev = list->prev;
//}

//用于管理空闲 tid
static struct kthread_map {
    uint8_t map[DIV_CEIL(KTHREAD_NUM, 8)];
    uint32_t len;
} tid_map = {
        .len = DIV_CEIL(KTHREAD_NUM, 8),
        .map = {0},
};


//cur_task 为循环链表

tcb_t *finish_task = NULL;   //已执行完成等待销毁的线程

tcb_t *runnable_task = NULL; //正在运行或已经运行完成的线程
tcb_t *block_task = NULL;    //阻塞中的线程
tcb_t *cleaner_task = NULL;  //清理线程,用于清理其他线程

spinlock_t block_lock;
spinlock_t unblock_lock;

// dest->prev 插入src节点
void list_inert_p(tcb_t *src, tcb_t *dest) {
    src->prev = dest->prev;
    src->next = dest;
    if (dest->prev != NULL) dest->prev->next = src;
    dest->prev = src;
}

// dest->next 插入src节点
void list_inert_n(tcb_t *src, tcb_t *dest) {
    src->prev = dest;
    src->next = dest->next;
    if (dest->next != NULL) dest->next->prev = src;
    dest->next = src;
}

__attribute__((always_inline))
static inline void list_delete(tcb_t **header, tcb_t *node) {
    if (*header == node) *header = node->next;
    else node->prev = node->next;
}

//在 target 中查找 node
tcb_t *list_find(tcb_t *target, kthread_t tid) {
    while (target != NULL) {
        if (target->tid == tid) return target;
        target = target->next;
    }
    return NULL;
}

//node 添加到 target 末尾
void list_append(tcb_t **target, tcb_t *node) {
    if (*target == NULL) {
        *target = node;
    } else {
        node->prev = *target;
        node->next = (*target)->next;
        (*target)->next = node;
        if ((*target)->next != NULL) (*target)->next->prev = node;
    }
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

static void *cleaner_worker(){
    // 主线程不会被添加到 finish_task
    while (finish_task != NULL) {
        tcb_t *next = finish_task->next;
        free_tid(finish_task->tid);
        freeK(finish_task->stack);
        finish_task = next;
    }
}


//初始化内核主线程
void sched_init() {
    thread_timer_init();
    spinlock_init(&block_lock);
    spinlock_init(&unblock_lock);
    register uint32_t esp asm("esp");
    cur_task = mallocK(sizeof(tcb_t));
    cur_task->prev = cur_task;
    cur_task->next = cur_task;
    cur_task->context.esp = esp;
    cur_task->tid = alloc_tid();
    cur_task->state = TASK_RUNNING_OR_RUNNABLE;

    kthread_create(cleaner_worker,NULL);
    cleaner_task = cur_task ->prev;
}


//void kthread_exit(void *ret)
void kthread_exit() {
    k_lock();
    finish_task->next = cur_task;
    cur_task->prev = cur_task->next;
    cur_task->state = TASK_ZOMBIE;
    block_thread(cur_task);
    unblock_thread(cleaner_task);
    k_unlock();
    idle();
}

extern void kthread_worker(void *(worker)(void *), void *args, tcb_t *tcb);


// 成功返回 0,否则返回错误码(<0)
int kthread_create(void *(worker)(void *), void *args) {
    //tcb 结构放到栈底
    void *stack = mallocK(KTHREAD_STACK_SIZE);
    assertk(stack != NULL);
    tcb_t *thread = stack + KTHREAD_STACK_SIZE - sizeof(tcb_t);
    thread->state = TASK_RUNNING_OR_RUNNABLE;
    thread->stack = stack;

    pointer_t *esp = (void *) thread - sizeof(pointer_t) * 4;
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
    //多 cpu 需要在这里加锁
    if (cur_task != NULL) {
        if (cur_task->next != cur_task) {
            cur_task = cur_task->next;
            switch_to(&cur_task->prev->context, &cur_task->context);
        }
    } else if (block_task != NULL) {
        // 所有线程都被阻塞
        halt();
    }
}

// 阻塞线程
void block_thread() {
    spinlock_lock(&block_lock);
    cur_task->state = TASK_SLEEPING;
    list_delete(&runnable_task, cur_task);
    list_append(&block_task, cur_task);
    schedule();
    spinlock_unlock(&block_lock);
}

void unblock_thread(tcb_t *thread) {
    spinlock_lock(&unblock_lock);
    list_delete(&block_task, thread);
    spinlock_unlock(&unblock_lock);
}
