//
// Created by pjs on 2021/2/19.
//
// TODO: join 等待子线程退出, tcb 添添加子线程列表

#include "sched/kthread.h"
#include "lib/spinlock.h"
#include "lib/qlib.h"
#include "sched/timer.h"
#include "lib/qstring.h"
#include "mm/kmalloc.h"
#include "lib/irlock.h"
#include "sched/schedule.h"

typedef struct thread_btm {
    void *entry;
    void *ret;     // entry 的返回地址
    void *worker;  // entry 参数
    void *args;    // entry 参数
} thread_btm_t;


#define THREAD_BTM(top) ((void*)(top) + KTHREAD_STACK_SIZE-sizeof(thread_btm_t))

//用于管理空闲 tid
// TODO: 建立 tid 哈希表
static u32_t kthread_map[KTHREAD_NUM];

static LIST_HEAD(finish_list);  //已执行完成等待销毁的线程
static LIST_HEAD(block_list);   //全局阻塞列表


list_head_t *init_task = NULL;     //空闲线程,始终在可运行列表
static list_head_t *cleaner_task = NULL;  //清理线程,用于清理其他线程

static kthread_t alloc_tid(tcb_t *tcb);

static void free_tid(kthread_t tid);

_Noreturn static void *cleaner_worker();

static void cleaner_thread_init();

static void init_thread_init();

extern void kthread_worker(void *(worker)(void *), void *args, tcb_t *tcb);


//初始化内核线程
void thread_init() {
#ifdef DEBUG
    cur_tcb()->magic = THREAD_MAGIC;
#endif //DEBUG
    thread_timer_init();
    init_thread_init();

    CUR_TCB->priority = MAX_PRIORITY;
    CUR_TCB->timer_slice = 100;
    CUR_TCB->tid = alloc_tid(CUR_TCB);
    assertk(CUR_TCB->tid != 0);
    CUR_TCB->state = TASK_RUNNING;
    CUR_TCB->stack = CUR_TCB;
    kthread_set_name(CUR_TCB->tid, "main");

    asm volatile("movl %%esp, %0":"=rm"(CUR_TCB->context.esp));
    cleaner_thread_init();
}


void kthread_exit() {
    ir_lock_t lock;
    ir_lock(&lock);

    list_add_next(&CUR_HEAD, &finish_list);
    tcb_t *ct = tcb_entry(cleaner_task);
    if (ct->state != TASK_RUNNING)
        unblock_thread(cleaner_task);

    CUR_TCB->state = TASK_ZOMBIE;
    schedule();

    ir_unlock(&lock);
    idle();
}


static kthread_t alloc_tid(tcb_t *tcb) {
    for (int i = 0; i < KTHREAD_NUM; ++i) {
        if (kthread_map[i] == 0) {
            kthread_map[i] = (ptr_t) tcb;
            return i;
        }
    }
    // 0 始终被 init 线程占用
    return 0;
}

static void free_tid(kthread_t tid) {
    assertk(tid != 0);
    kthread_map[tid] = 0;
}


_Noreturn static void *cleaner_worker() {
    while (1) {
        ir_lock_t lock;
        ir_lock(&lock);

        list_head_t *hdr, *next;
        list_for_each_del(hdr, next, &finish_list) {
            tcb_t *entry = tcb_entry(hdr);
            assertk(entry->state == TASK_ZOMBIE);
            next = hdr->next;
            free_tid(entry->tid);
            kfree(entry->stack);
        }
        list_header_init(&finish_list);
        block_thread(NULL, NULL);

        ir_unlock(&lock);
    }
}

static void cleaner_thread_init() {
    kthread_t tid;
    kthread_create(&tid, cleaner_worker, NULL);
    cleaner_task = kthread_get_run_list(tid);
    kthread_set_name(tid, "cleaner");
}

static void *init_worker() {
    idle();
}


static void init_thread_init() {
    kthread_t tid;
    kthread_create(&tid, init_worker, NULL);
    assertk(tid == 0);
    init_task = kthread_get_run_list(tid);
    kthread_set_name(tid, "init");
    kthread_set_time_slice(tid, 0);
}

// 成功返回 0,否则返回错误码(<0)
int kthread_create(kthread_t *tid, void *(worker)(void *), void *args) {
    //tcb 结构放到栈顶(低地址)
    ir_lock_t lock;
    ir_lock(&lock);
    tcb_t *thread = kmalloc(PAGE_SIZE);
    ir_unlock(&lock);

    assertk(thread != NULL);
    assertk(((ptr_t) thread & PAGE_MASK) == 0);

    thread->state = TASK_RUNNING;
    thread->stack = thread;
#ifdef DEBUG
    thread->magic = THREAD_MAGIC;
#endif// DEBUG

    thread_btm_t *btm = THREAD_BTM(thread);
    btm->args = args;
    btm->worker = worker;
    btm->ret = NULL;
    btm->entry = kthread_worker;

    thread->context.esp = (ptr_t) btm;
    // 必须关闭中断,且 schedule 必须关中断,否则 switch 函数恢复 eflags
    // 可能由于中断切换,导致 esp 还没有恢复(switch 时使用了临时 esp 用于保存 context)
    thread->context.eflags = 0;
    thread->name[0] = '\0';
    thread->priority = MAX_PRIORITY;
    thread->timer_slice = TIME_SLICE_LENGTH;

    ir_lock(&lock);
    thread->tid = alloc_tid(thread);
    *tid = thread->tid;

    sched_task_add(&thread->run_list);
    ir_unlock(&lock);
    return 0;
}

void kthread_set_time_slice(kthread_t tid, u16_t time_slice) {
    tcb_t *tcb;
    assertk(tid < KTHREAD_NUM);
    tcb = (tcb_t *) kthread_map[tid];
    assertk(tcb->tid == tid);

#ifdef DEBUG
    assertk(tcb->magic == THREAD_MAGIC);
#endif //DEBUG
    tcb->timer_slice = time_slice;
}

void kthread_set_name(kthread_t tid, const char *name) {
    tcb_t *tcb;
    u8_t cnt;
    assertk(tid < KTHREAD_NUM);
    tcb = (tcb_t *) kthread_map[tid];
    assertk(tcb->tid == tid);
    cnt = q_strlen(name);

    assertk(cnt <= KTHREAD_NAME_LEN);
#ifdef DEBUG
    assertk(tcb->magic == THREAD_MAGIC);
#endif //DEBUG
    q_memcpy(tcb->name, name, cnt);
}

list_head_t *kthread_get_run_list(kthread_t tid) {
    tcb_t *tcb;
    assertk(tid < KTHREAD_NUM);
    tcb = (tcb_t *) kthread_map[tid];
    assertk(tcb->tid == tid);

#ifdef DEBUG
    assertk(tcb->magic == THREAD_MAGIC);
#endif //DEBUG
    return &tcb->run_list;
}

// 睡眠前释放锁,被唤醒后自动获取锁, 传入的锁没有被获取则不会睡眠
int8_t block_thread(list_head_t *_block_list, spinlock_t *lk) {
    ir_lock_t lock;
    ir_lock(&lock);
    if (!_block_list)
        _block_list = &block_list;
    if (lk) {
        if (lk->flag == SPINLOCK_UNLOCK) {
            ir_unlock(&lock);
            return -1;
        };
        spinlock_unlock(lk);
    }

    list_add_next(&CUR_HEAD, _block_list);

    CUR_TCB->state = TASK_SLEEPING;
    schedule();

    if (lk) spinlock_lock(lk);
    ir_unlock(&lock);
    return 0;
}

void unblock_thread(list_head_t *_thread) {
    ir_lock_t lock;
    ir_lock(&lock);

    //TODO:如果线程在计时器睡眠队列,将该计时器删除
    tcb_t *thread = tcb_entry(_thread);
    if (thread->state != TASK_RUNNING) {
        thread->state = TASK_RUNNING;
        list_del(&thread->run_list);
        sched_task_add(&thread->run_list);
    }

    ir_unlock(&lock);
}

// ================线程测试
#ifdef TEST

spinlock_t lock;
int32_t foo[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

void *workerA(UNUSED void *args) {
    spinlock_lock(&lock);
    printfk("lock, %d\n", (*(int32_t *) args));
    spinlock_unlock(&lock);
    return NULL;
}

void test_thread() {
    test_start;
    spinlock_init(&lock);
    kthread_t tid[10];
    const char *name[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9"};
    for (int i = 0; i < 10; ++i) {
        kthread_create(&tid[i], workerA, &foo[i]);
        kthread_set_name(tid[i], name[i]);
    }
    test_pass;
}

#endif //TEST