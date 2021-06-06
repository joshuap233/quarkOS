//
// Created by pjs on 2021/6/3.
//

#include <lib/list.h>
#include <lib/qstring.h>
#include <fs/vfs.h>
#include <mm/kvm.h>
#include <mm/kmalloc.h>
#include <mm/vmalloc.h>
#include <sched/task.h>
#include <sched/schedule.h>
#include <sched/fork.h>
#include <sched/timer.h>

//用于管理空闲 tid
// TODO: 建立 tid 哈希表
static tcb_t *task_map[TASK_NUM];

static LIST_HEAD(finish_list);            // 已执行完成等待销毁的线程
static LIST_HEAD(block_list);             // 全局阻塞列表
list_head_t *idle_task = NULL;            // 空闲线程,始终在可运行列表
static list_head_t *cleaner_task = NULL;  // 清理线程,用于清理其他线程

static void cleaner_task_init();

extern void task_entry();

static pid_t alloc_pid(tcb_t *tcb);

_Noreturn static void *cleaner_worker();

static void idle_task_init();

static void free_pid(pid_t pid);


typedef struct thread_bottom {
    void *entry;
    void *ret;     // entry 的返回地址
    void *worker;  // entry 参数
    void *args;    // entry 参数
} thread_bottom;


void task_init() {
#ifdef DEBUG
    cur_tcb()->magic = TASK_MAGIC;
#endif //DEBUG
    thread_timer_init();
    idle_task_init();

    CUR_TCB->priority = TASK_MAX_PRIORITY;
    // 任务初始化后,还有内核模块需要初始化,因此设置一个很大的时间片
    CUR_TCB->timer_slice = 10000;
    CUR_TCB->pid = alloc_pid(CUR_TCB);
    assertk(CUR_TCB->pid != 0);
    CUR_TCB->state = TASK_RUNNING;
    CUR_TCB->stack = CUR_TCB;
    task_set_name(CUR_TCB->pid, "init");

    asm volatile("movl %%esp, %0":"=rm"(CUR_TCB->context.esp));
    cleaner_task_init();
}


struct task_struct *kernel_clone(struct task_struct *cur, u32_t flag) {
    ir_lock_t lock;
    ir_lock(&lock);
    tcb_t *task = kmalloc(PAGE_SIZE);
    ir_unlock(&lock);
    assertk(task != NULL);

#ifdef DEBUG
    task->magic = TASK_MAGIC;
#endif //DEBUG

    task->state = TASK_RUNNING;
    task->priority = cur->priority;
    task->timer_slice = TIME_SLICE_LENGTH;
    task->name[0] = '\0';

    task->stack = task;
    if (flag & CLONE_KERNEL_STACK)
        q_memcpy(task->stack + sizeof(tcb_t),
                 cur->stack + sizeof(tcb_t),
                 PAGE_SIZE - sizeof(tcb_t));

    task->mm = NULL;
    if (flag & CLONE_MM && cur->mm)
        task->mm = vm_struct_copy(cur->mm);

    task->cwd = cur->cwd;

    if (flag & CLONE_CONTEXT)
        q_memcpy(&task->context,
                 &cur->context, sizeof(context_t));

    ir_lock(&lock);
    task->pid = alloc_pid(task);
//    sched_task_add(&task->run_list);
    ir_unlock(&lock);
    return task;
}

void user_task_init() {
    extern void goto_usermode(u32_t stack_bottom);
    extern char user_text_start[], user_rodata_start[],
            user_data_start[], user_bss_end[];
    ptr_t text = (ptr_t) user_text_start;
    ptr_t rodata = (ptr_t) user_rodata_start;
    ptr_t data = (ptr_t) user_data_start;
    ptr_t bssEnd = (ptr_t) user_bss_end;

    struct mm_struct *mm = mm_struct_init(
            text, data - text,
            rodata, data - rodata,
            data, bssEnd - data
    );

    kvm_copy(mm->pgdir);
    CUR_TCB->mm = mm;
    switch_uvm(mm->pgdir);
    goto_usermode(mm->stack.addr + PAGE_SIZE);
}

pid_t fork() {
    struct task_struct *thread = kernel_clone(CUR_TCB, CLONE_MM | CLONE_CONTEXT | CLONE_KERNEL_STACK);
    sched_task_add(&thread->run_list);
    return 0;
}

extern void thread_entry(void *(worker)(void *), void *args, tcb_t *tcb);

// 成功返回 0,否则返回错误码(<0)
int kthread_create(kthread_t *tid, void *(worker)(void *), void *args) {
    struct task_struct *thread = kernel_clone(CUR_TCB, 0);
    thread_bottom *bottom = ((void *) thread + STACK_SIZE - sizeof(thread_bottom));
    bottom->args = args;
    bottom->worker = worker;
    bottom->ret = NULL;
    bottom->entry = thread_entry;

    thread->context.eflags = 0;
    thread->context.esp = (ptr_t) bottom;
    *tid = thread->pid;
    sched_task_add(&thread->run_list);
    return 0;
}

static pid_t alloc_pid(tcb_t *tcb) {
    for (int i = 0; i < TASK_NUM; ++i) {
        if (task_map[i] == 0) {
            task_map[i] = tcb;
            return i;
        }
    }
    // 0 始终被 idle 线程占用
    return 0;
}


static void free_pid(pid_t pid) {
    assertk(pid != 0);
    task_map[pid] = NULL;
}

static void *idle_worker() {
    idle();
}

void task_set_name(pid_t pid, const char *name) {
    assertk(pid < TASK_NUM);

    ir_lock_t lock;
    tcb_t *tcb;
    u8_t cnt;

    ir_lock(&lock);
    tcb = task_map[pid];
    if (tcb) {
        cnt = q_strlen(name);

        assertk(cnt <= TASK_NAME_LEN);
#ifdef DEBUG
        assertk(tcb->magic == TASK_MAGIC);
#endif //DEBUG
        q_memcpy(tcb->name, name, cnt);
    }

    ir_unlock(&lock);
}

list_head_t *task_get_run_list(pid_t pid) {
    assertk(pid < TASK_NUM);

    tcb_t *tcb;
    tcb = task_map[pid];

#ifdef DEBUG
    assertk(tcb->magic == TASK_MAGIC);
#endif //DEBUG
    return &tcb->run_list;
}

void task_set_time_slice(pid_t pid, u16_t time_slice) {
    tcb_t *tcb;
    assertk(pid < TASK_NUM);
    tcb = task_map[pid];
    assertk(tcb->pid == pid);

#ifdef DEBUG
    assertk(tcb->magic == TASK_MAGIC);
#endif //DEBUG
    tcb->timer_slice = time_slice;
}

static void idle_task_init() {
    kthread_t tid;
    kthread_create(&tid, idle_worker, NULL);
    assertk(tid == 0);
    idle_task = task_get_run_list(tid);
    task_set_name(tid, "idle");
    task_set_time_slice(tid, 0);
}


static void cleaner_task_init() {
    kthread_t tid;
    kthread_create(&tid, cleaner_worker, NULL);
    cleaner_task = task_get_run_list(tid);
    task_set_name(tid, "cleaner");
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
            free_pid(entry->pid);
            kfree(entry->stack);
            //TODO: 回收打开的文件
            if (entry->mm) {
                vm_struct_destroy(entry->mm);
            }
        }
        list_header_init(&finish_list);
        task_sleep(NULL, NULL);

        ir_unlock(&lock);
    }
}

// 睡眠前释放锁,被唤醒后自动获取锁, 传入的锁没有被获取则不会睡眠
int8_t task_sleep(list_head_t *_block_list, spinlock_t *lk) {
    ir_lock_t lock;
    ir_lock(&lock);
    if (!_block_list)
        _block_list = &block_list;
    if (lk) {
        if (!spinlock_locked(&lock)) {
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

void task_wakeup(list_head_t *_task) {
    ir_lock_t lock;
    ir_lock(&lock);

    //TODO:如果线程在计时器睡眠队列,将该计时器删除
    tcb_t *task = tcb_entry(_task);
    if (task->state != TASK_RUNNING) {
        task->state = TASK_RUNNING;
        list_del(&task->run_list);
        sched_task_add(&task->run_list);
    }

    ir_unlock(&lock);
}

void task_exit() {
    ir_lock_t lock;
    ir_lock(&lock);

    list_add_next(&CUR_HEAD, &finish_list);
    tcb_t *ct = tcb_entry(cleaner_task);
    if (ct->state != TASK_RUNNING)
        task_wakeup(cleaner_task);

    CUR_TCB->state = TASK_ZOMBIE;
    schedule();

    ir_unlock(&lock);
    idle();
}


// ================线程测试
#ifdef TEST

static spinlock_t lock;
static int32_t foo[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

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
    for (int i = 0; i < 9; ++i) {
        kthread_create(&tid[i], workerA, &foo[i]);
        task_set_name(tid[i], name[i]);
    }
    test_pass;
}

#endif //TEST