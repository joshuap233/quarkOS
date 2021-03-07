//
// Created by pjs on 2021/2/19.
//
#include "sched/kthread.h"
#include "sched/klock.h"
#include "mm/heap.h"
#include "klib/qlib.h"
#include "klib/qmath.h"
#include "sched/timer.h"
#include "mm/mm.h"
#include "klib/qstring.h"

typedef struct thread_btm {
    void *entry;
    void *ret;     // entry 的返回地址
    void *worker;  // entry 参数
    void *args;    // entry 参数
} thread_btm_t;


#define THREAD_BTM(top) ((void*)(top) + KTHREAD_STACK_SIZE-sizeof(thread_btm_t))


static void _list_header_init(tcb_t *header) {
    header->next = header;
    header->prev = header;
}

__attribute__((always_inline))
static inline void _list_add(tcb_t *new, tcb_t *prev, tcb_t *next) {
    new->prev = prev;
    new->next = next;
    next->prev = new;
    prev->next = new;
}

__attribute__((always_inline))
static inline void _list_add_next(tcb_t *new, tcb_t *target) {
    _list_add(new, target, target->next);
}

__attribute__((always_inline))
static inline void _list_add_prev(tcb_t *new, tcb_t *target) {
    _list_add(new, target->prev, target);
}


__attribute__((always_inline))
static inline void _list_del(tcb_t *list) {
    list->prev->next = list->next;
    list->next->prev = list->prev;
}

__attribute__((always_inline))
static inline bool _list_empty(tcb_t *header) {
    return header->next == header;
}

//用于管理空闲 tid
static struct kthread_map {
    uint8_t map[DIV_CEIL(KTHREAD_NUM, 8)];
    uint32_t len;
} tid_map = {
        .len = DIV_CEIL(KTHREAD_NUM, 8),
        .map = {0},
};

tcb_t finish_list;   //已执行完成等待销毁的线程
tcb_t block_list;    //阻塞中的线程

tcb_t *cleaner_task = NULL;  //清理线程,用于清理其他线程
tcb_t *init_task = NULL;     //空闲线程,时钟在可运行列表
tcb_t *ready_to_run = NULL;  //指向运行队列节点

static kthread_t alloc_tid();

static void free_tid(kthread_t tid);

static void *idle_worker();

_Noreturn static void *cleaner_worker();

static void _block_thread(kthread_state_t state);

static void cleaner_thread_init();

// 空闲线程作为头节点加入循环链表，该线程永远在可运行线程列表
//添加 idle task 方便任务列表的管理
static void init_thread_init();

static int _kthread_create(tcb_t **_thread, kthread_t *tid, void *(worker)(void *), void *args);


extern void kthread_worker(void *(worker)(void *), void *args, tcb_t *tcb);

static inline void set_next_ready() {
    //从运行队列删除节点前需要调用该方法
    ready_to_run = CUR_TCB->next;
}

static inline void del_cur_task() {
    set_next_ready();
    _list_del(CUR_TCB);
}

//初始化内核线程
void sched_init() {
    thread_timer_init();
    init_thread_init();

    _list_header_init(&finish_list);
    _list_header_init(&block_list);

    CUR_TCB->tid = alloc_tid();
    CUR_TCB->state = TASK_RUNNING;
    CUR_TCB->stack = CUR_TCB;
    q_memcpy(CUR_TCB->name, "main", sizeof("main"));
    asm volatile("movl %%esp, %0":"=rm"(CUR_TCB->context.esp));
    _list_add_prev(CUR_TCB, init_task);
    cleaner_thread_init();
    set_next_ready();
}


void kthread_exit() {
    ir_lock_t lock;
    ir_lock(&lock);

    del_cur_task();
    _list_add_next(CUR_TCB, &finish_list);
    if (cleaner_task->state != TASK_RUNNING)
        unblock_thread(cleaner_task);
    _block_thread(TASK_ZOMBIE);

    ir_unlock(&lock);
    idle();
}


int kthread_create(kthread_t *tid, void *(worker)(void *), void *args) {
    tcb_t *thread;
    return _kthread_create(&thread, tid, worker, args);
}

void schedule() {
    tcb_t *next = ready_to_run;
    if (CUR_TCB == init_task && next == init_task)
        return;
    if (next == init_task) {
        if (next->next == CUR_TCB) return;
        next = next->next;
    }
    ready_to_run = next->next;
    switch_to(&CUR_TCB->context, &next->context);
}

static void _block_thread(kthread_state_t state) {
    CUR_TCB->state = state;
    schedule();
}

void block_thread() {
    ir_lock_t lock;
    ir_lock(&lock);

    del_cur_task();
    _list_add_next(CUR_TCB, &block_list);
    _block_thread(TASK_SLEEPING);

    ir_unlock(&lock);
}

void unblock_thread(tcb_t *thread) {
    ir_lock_t lock;
    ir_lock(&lock);

    thread->state = TASK_RUNNING;
    _list_del(thread);
    _list_add_prev(thread, init_task);

    ir_unlock(&lock);
}

static kthread_t alloc_tid() {
    for (uint32_t index = 0; index < tid_map.len; ++index) {
        uint8_t value = tid_map.map[index];
        for (int bit = 0; bit < 8; ++bit, value >>= 1) {
            if (!(value & 0b1)) {
                set_bit(&tid_map.map[index], bit);
                return index * 8 + bit;
            }
        }
    }
    // 0 始终被 init 线程占用
    return 0;
}

static void free_tid(kthread_t tid) {
    assertk(tid != 0);
    clear_bit(&tid_map.map[tid / 8], tid % 8);
}

static void *idle_worker() {
    idle();
}

_Noreturn static void *cleaner_worker() {

    while (1){
        ir_lock_t lock;
        ir_lock(&lock);

        tcb_t *next;
        for (tcb_t *hdr = finish_list.next; hdr != &finish_list; hdr = next) {
            next = hdr;
            free_tid(hdr->tid);
            freeK(hdr->stack);
        }
        _list_header_init(&finish_list);
        block_thread();

        ir_unlock(&lock);
    }
}

static void cleaner_thread_init() {
    kthread_t tid;
    _kthread_create(&cleaner_task, &tid, cleaner_worker, NULL);
    q_memcpy(cleaner_task->name, "cleaner", sizeof("cleaner"));
}

// 空闲线程作为头节点加入循环链表，该线程永远在可运行线程列表
//添加 idle task 方便任务列表的管理
static void init_thread_init() {
    kthread_t tid;
    _kthread_create(&init_task, &tid, idle_worker, NULL);
    _list_header_init(init_task);
    q_memcpy(init_task->name, "init", sizeof("init"));
    assertk(init_task->tid == 0);
}

// 成功返回 0,否则返回错误码(<0)
static int _kthread_create(tcb_t **_thread, kthread_t *tid, void *(worker)(void *), void *args) {
    //tcb 结构放到栈顶(低地址)
    tcb_t *thread = allocK_page();
    *_thread = thread;
    assertk(thread != NULL);
    assertk(((pointer_t) thread & ALIGN_MASK) == 0);

    thread->state = TASK_RUNNING;
    thread->stack = thread;

    thread_btm_t *btm = THREAD_BTM(thread);
    btm->args = args;
    btm->worker = worker;
    btm->ret = NULL;
    btm->entry = kthread_worker;

    thread->context.esp = (pointer_t) btm;
    thread->context.eflags = get_eflags() | INTERRUPT_MASK; // 开启中断
    thread->name[0] = '\0';

    ir_lock_t lock;
    ir_lock(&lock);

    thread->tid = alloc_tid();
    *tid = thread->tid;
    _list_add_prev(thread, CUR_TCB);

    ir_unlock(&lock);
    return 0;
}