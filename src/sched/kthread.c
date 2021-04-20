//
// Created by pjs on 2021/2/19.
//
// 使用 LIST_DEL, 不要直接使用 list_del!!!

#include "sched/kthread.h"
#include "sched/klock.h"
#include "lib/qlib.h"
#include "sched/timer.h"
#include "lib/qstring.h"
#include "mm/heap.h"


typedef struct thread_btm {
    void *entry;
    void *ret;     // entry 的返回地址
    void *worker;  // entry 参数
    void *args;    // entry 参数
} thread_btm_t;


#define THREAD_BTM(top) ((void*)(top) + KTHREAD_STACK_SIZE-sizeof(thread_btm_t))


//用于管理空闲 tid
static struct kthread_map {
    uint8_t map[DIV_CEIL(KTHREAD_NUM, 8)];
    uint32_t len;
} tid_map = {
        .len = DIV_CEIL(KTHREAD_NUM, 8),
        .map = {0},
};

LIST_HEAD(finish_list);  //已执行完成等待销毁的线程
LIST_HEAD(block_list);   //阻塞中的线程


list_head_t *cleaner_task = NULL;  //清理线程,用于清理其他线程
list_head_t *init_task = NULL;     //空闲线程,始终在可运行列表
list_head_t *ready_to_run = NULL;  //指向运行队列节点

extern void switch_to(context_t *cur_context, context_t *next_context);

static kthread_t alloc_tid();

static void free_tid(kthread_t tid);

static void *init_worker();

_Noreturn static void *cleaner_worker();

static void kt_block(kthread_state_t state);

static void cleaner_thread_init();

// 空闲线程作为头节点加入循环链表，该线程永远在可运行线程列表
//添加 idle task 方便任务列表的管理
static void init_thread_init();

static int kt_create(list_head_t **_thread, kthread_t *tid, void *(worker)(void *), void *args);

extern void kthread_worker(void *(worker)(void *), void *args, tcb_t *tcb);

INLINE void unblock(list_head_t *head);

_Noreturn INLINE void idle();

INLINE void set_next_ready();

#define LIST_DEL(l) {set_next_ready();list_del(l);}
#define del_cur_task() LIST_DEL(&CUR_HEAD);

//初始化内核线程
void sched_init() {
    thread_timer_init();
    init_thread_init();

    CUR_TCB->tid = alloc_tid();
    CUR_TCB->state = TASK_RUNNING;
    CUR_TCB->stack = CUR_TCB;
    q_memcpy(CUR_TCB->name, "main", sizeof("main"));
    asm volatile("movl %%esp, %0":"=rm"(CUR_TCB->context.esp));
    list_add_prev(&CUR_HEAD, init_task);
    cleaner_thread_init();
    set_next_ready();
}


void kthread_exit() {
    ir_lock_t lock;
    ir_lock(&lock);

    del_cur_task();
    list_add_next(&CUR_HEAD, &finish_list);
    tcb_t *ct = tcb_entry(cleaner_task);
    if (ct->state != TASK_RUNNING)
        unblock_thread(cleaner_task);
    kt_block(TASK_ZOMBIE);

    ir_unlock(&lock);
    idle();
}


int kthread_create(kthread_t *tid, void *(worker)(void *), void *args) {
    list_head_t *thread;
    return kt_create(&thread, tid, worker, args);
}

void schedule() {
    ir_lock_t lock;
    ir_lock(&lock);
    list_head_t *next = ready_to_run;
    if (&CUR_HEAD == init_task && init_task->next == init_task)
        return;

    if (next == init_task) {
        if (next->next == &CUR_HEAD) return;
        next = next->next;
    }
    ready_to_run = next->next;

    g_time_slice = next == init_task ? 0 : TIME_SLICE_LENGTH;
    switch_to(&CUR_TCB->context, &tcb_entry(next)->context);
    ir_unlock(&lock);
}

static void kt_block(kthread_state_t state) {
    CUR_TCB->state = state;
    schedule();
}

// 睡眠前释放锁,被唤醒后自动获取锁, 传入的锁没有被获取则不会睡眠
int8_t block_thread(list_head_t *_block_list, spinlock_t *lk) {
    ir_lock_t lock;
    ir_lock(&lock);
    if (lk) {
        if (lk->flag == 0) {
            ir_unlock(&lock);
            return -1;
        };
        spinlock_unlock(lk);
    }

    del_cur_task();
    list_add_next(&CUR_HEAD, _block_list);
    kt_block(TASK_SLEEPING);

    if (lk) spinlock_lock(lk);
    ir_unlock(&lock);
    return 0;
}

INLINE void unblock(list_head_t *head) {
    tcb_t *thread = tcb_entry(head);
    thread->state = TASK_RUNNING;
    LIST_DEL(&thread->run_list);
    list_add_prev(&thread->run_list, init_task);
}

void unblock_thread(list_head_t *head) {
    ir_lock_t lock;
    ir_lock(&lock);
    unblock(head);
    ir_unlock(&lock);
}

// 唤醒从 head 开始的所有线程, head 为指针头(不是有效的 tcb)
void unblock_threads(list_head_t *head) {
    ir_lock_t lock;
    ir_lock(&lock);
    list_for_each_del(head) {
        unblock(hdr);
    }
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

// 所有线程睡眠时才会执行
static void *init_worker() {
    idle();
}

_Noreturn static void *cleaner_worker() {
    while (1) {
        ir_lock_t lock;
        ir_lock(&lock);

        list_for_each_del(&finish_list) {
            tcb_t *entry = tcb_entry(hdr);
            next = hdr->next;
            free_tid(entry->tid);
            freeK(entry->stack);
        }
        list_header_init(&finish_list);
        block_thread(&block_list, NULL);

        ir_unlock(&lock);
    }
}

static void cleaner_thread_init() {
    kthread_t tid;
    kt_create(&cleaner_task, &tid, cleaner_worker, NULL);
    q_memcpy(tcb_entry(cleaner_task)->name, "cleaner", sizeof("cleaner"));
}

// 空闲线程作为头节点加入循环链表，该线程永远在可运行线程列表
//添加 idle task 方便任务列表的管理
static void init_thread_init() {
    kthread_t tid;
    kt_create(&init_task, &tid, init_worker, NULL);
    list_header_init(init_task);
    q_memcpy(tcb_entry(init_task)->name, "init", sizeof("init"));
    assertk(tcb_entry(init_task)->tid == 0);
}

// 成功返回 0,否则返回错误码(<0)
static int kt_create(list_head_t **_thread, kthread_t *tid, void *(worker)(void *), void *args) {
    //tcb 结构放到栈顶(低地址)
    ir_lock_t lock;
    ir_lock(&lock);
    tcb_t *thread = allocK_page();
    ir_unlock(&lock);

    *_thread = &thread->run_list;
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
    // 必须关闭中断,且 schedule 必须关中断,否则 switch 函数恢复 eflags
    // 可能由于中断切换,导致 esp 还没有恢复(switch 时使用了临时 esp 用于保存 context)
    thread->context.eflags = 0;
    thread->name[0] = '\0';

    ir_lock(&lock);
    thread->tid = alloc_tid();
    *tid = thread->tid;
    list_add_prev(&thread->run_list, &CUR_HEAD);
    ir_unlock(&lock);
    return 0;
}


_Noreturn INLINE void idle() {
    while (1) {
        halt();
    }
}


INLINE void set_next_ready() {
    //从运行队列删除节点前必需调用该方法
    ready_to_run = CUR_HEAD.next;
}


// ================线程测试
#ifdef TEST

spinlock_t lock;

void *workerA(UNUSED void *args) {
    spinlock_lock(&lock);
    printfk("lock\n");
    spinlock_unlock(&lock);
    return NULL;
}

void test_thread() {
    test_start;
    spinlock_init(&lock);
    kthread_t a[10];
    for (int i = 0; i < 2; ++i) {
        kthread_create(&a[i], workerA, NULL);
    }
    test_pass;
}

#endif //TEST