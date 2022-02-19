//
// Created by pjs on 2021/6/3.
//

#include <lib/list.h>
#include <lib/qstring.h>
#include <fs/vfs.h>
#include <mm/vm.h>
#include <mm/kmalloc.h>
#include <mm/uvm.h>
#include <task/task.h>
#include <task/schedule.h>
#include <task/fork.h>
#include <task/timer.h>
#include <cpu.h>

//用于管理空闲 tid
static tcb_t *task_map[TASK_NUM];

static LIST_HEAD(finish_list);            // 已执行完成等待销毁的线程
static LIST_HEAD(block_list);             // 全局阻塞列表
static list_head_t *cleaner_task = NULL;  // 清理线程,用于清理其他线程

static void cleaner_task_init();

static pid_t alloc_pid(tcb_t *tcb);

_Noreturn static void *cleaner_worker();

static void free_pid(pid_t pid);


void task_init1(){
#ifdef DEBUG
    cur_tcb()->magic = TASK_MAGIC;
#endif //DEBUG

    CUR_TCB->priority = TASK_MAX_PRIORITY;
    // 任务初始化后,还有内核模块需要初始化,因此设置一个很大的时间片
    // CUR_TCB->timer_slice = 10000;
    CUR_TCB->timer_slice = TIME_SLICE_LENGTH;
    CUR_TCB->pid = alloc_pid(CUR_TCB);
    CUR_TCB->state = TASK_RUNNING;
    CUR_TCB->stack = CUR_TCB;

    CUR_TCB->sysContext = NULL;
    CUR_TCB->context = NULL;
    CUR_TCB->open = NULL;
    task_set_name(CUR_TCB->pid, "idle");
    spinlock_init(&CUR_TCB->lock);
    getCpu()->idle = &CUR_TCB->run_list;
}

void task_init() {
    task_init1();
    thread_timer_init();
    cleaner_task_init();
}

void user_task_init() {
    extern void set_tss_esp(void *stack);
    extern void goto_usermode(u32_t stack_bottom);
    extern char user_text_start[], user_rodata_start[],
            user_data_start[], user_bss_start[], user_bss_end[];
    ptr_t text = (ptr_t) user_text_start;
    ptr_t rodata = (ptr_t) user_rodata_start;
    ptr_t data = (ptr_t) user_data_start;
    ptr_t bss = (ptr_t) user_bss_start;
    ptr_t bssEnd = (ptr_t) user_bss_end;

    struct mm_struct *mm = kmalloc(sizeof(struct mm_struct));
    vm_map_init(mm);
    struct mm_args args = {
            .text = text,
            .pa1 = text,
            .size1 = rodata - text,

            .rodata = rodata,
            .pa2 = rodata,
            .size2 = data - rodata,

            .data = data,
            .pa3 = data,
            .size3 = bss - data,

            .bss= bss,
            .pa4 = bss,
            .size4 = bssEnd - bss
    };
    mm_struct_init(mm, &args);
    spinlock_init(&CUR_TCB->lock);

    kvm_copy(mm->pgdir);
    CUR_TCB->mm = mm;
    CUR_TCB->cwd = vfs_ops.find("/");

    switch_uvm(mm->pgdir);
    set_tss_esp(CUR_TCB->stack + PAGE_SIZE);
    CUR_TCB->timer_slice = TIME_SLICE_LENGTH;

    goto_usermode(mm->stack.va + PAGE_SIZE);
}

struct task_struct *kernel_clone(struct task_struct *cur, u32_t flag) {
    extern void syscall_ret();
    ir_lock();
    tcb_t *task = kmalloc(PAGE_SIZE);
    assertk(task != NULL);
    task->pid = alloc_pid(task);
    ir_unlock();

#ifdef DEBUG
    task->magic = TASK_MAGIC;
#endif //DEBUG

    task->state = TASK_RUNNING;
    task->priority = cur->priority;
    task->timer_slice = TIME_SLICE_LENGTH;
    task->name[0] = '\0';
    task->mm = NULL;
    task->cwd = cur->cwd;
    task->stack = task;
    task->context = NULL;
    task->sysContext = NULL;
    task->open = NULL;

    if (flag & CLONE_FILE) {
        copy_open_file(&cur->open, &task->open);
    }

    if (flag & CLONE_MM) {
        assertk(cur->mm);
        task->mm = vm_struct_copy(cur->mm);
    }

    if (flag & CLONE_KERNEL_STACK) {
        // fork 的线程将会直接从 syscall_ret 开始执行
        ptr_t sysContext = (ptr_t) cur->sysContext;
        u32_t offset = sysContext & PAGE_MASK;

        memcpy(task->stack + offset,
               cur->stack + offset,
               STACK_SIZE - offset);
        task->context = task->stack + offset - sizeof(context_t);
        task->context->eip = (ptr_t) syscall_ret;
        task->context->eflags = 0x200; // 开启中断
        task->context->ebx = 0;        // 返回值(syscall_ret 还原到 eax)
    }

    spinlock_init(&task->lock);
    return task;
}


pid_t kernel_fork() {
    struct task_struct *task = kernel_clone(CUR_TCB, CLONE_MM | CLONE_KERNEL_STACK | CLONE_FILE);
    sched_task_add(&task->run_list);
    return task->pid;
}

extern void thread_entry(void *(worker)(void *), void *args, tcb_t *tcb);

// 成功返回 0,否则返回错误码(<0)
int kthread_create(kthread_t *tid, void *(worker)(void *), void *args) {
    struct task_struct *thread = kernel_clone(CUR_TCB, 0);
    struct context *context;
    context = (void *) thread + STACK_SIZE - sizeof(struct context);
    context->eflags = 0;
    context->ebx = (ptr_t) worker;
    context->ebp = (ptr_t) args;
    context->eip = (ptr_t) thread_entry;

    thread->context = context;
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

void task_set_name(pid_t pid, const char *name) {
    assertk(pid < TASK_NUM);

    tcb_t *tcb;
    u8_t cnt;

    ir_lock();
    tcb = task_map[pid];
    if (tcb) {
        cnt = strlen(name);

        assertk(cnt <= TASK_NAME_LEN);
#ifdef DEBUG
        assertk(tcb->magic == TASK_MAGIC);
#endif //DEBUG
        memcpy(tcb->name, name, cnt);
    }

    ir_unlock();
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

static void cleaner_task_init() {
    kthread_t tid;
    kthread_create(&tid, cleaner_worker, NULL);
    cleaner_task = task_get_run_list(tid);
    task_set_name(tid, "cleaner");
}


_Noreturn static void *cleaner_worker() {
    while (1) {
        ir_lock();

        list_head_t *hdr, *next;
        list_for_each_del(hdr, next, &finish_list) {
            tcb_t *task = tcb_entry(hdr);
            assertk(task->state == TASK_ZOMBIE);
            free_pid(task->pid);
            if (task->open) {
                recycle_open_file(task->open);
            }
            if (task->mm) {
                vm_struct_destroy(task->mm);
            }
            kfree(task->stack);
        }
        list_header_init(&finish_list);
        task_sleep(NULL, NULL);

        ir_unlock();
    }
}

// 睡眠前释放锁,被唤醒后自动获取锁, 传入的锁没有被获取则不会睡眠
int8_t task_sleep(list_head_t *_block_list, spinlock_t *lk) {
    ir_lock();
    if (!_block_list)
        _block_list = &block_list;
    if (lk) {
        if (!spinlock_locked(lk)) {
            ir_unlock();
            return -1;
        };
        spinlock_unlock(lk);
    }

    list_add_next(&CUR_HEAD, _block_list);

    CUR_TCB->state = TASK_SLEEPING;
    schedule();

    if (lk) spinlock_lock(lk);
    ir_unlock();
    return 0;
}

void task_wakeup(list_head_t *_task) {
    //TODO:如果线程在计时器睡眠队列,将该计时器删除
    tcb_t *task = tcb_entry(_task);
    spinlock_lock(&task->lock);
    if (task->state != TASK_RUNNING) {
        task->state = TASK_RUNNING;
        list_del(&task->run_list);
        sched_task_add(&task->run_list);
    }
    spinlock_unlock(&task->lock);
}

void task_exit() {
    ir_lock();

    assertk(&CUR_TCB->run_list != getCpu()->idle);
    list_add_next(&CUR_HEAD, &finish_list);
    tcb_t *ct = tcb_entry(cleaner_task);
    if (ct->state != TASK_RUNNING)
        task_wakeup(cleaner_task);

    CUR_TCB->state = TASK_ZOMBIE;
    schedule();
    ir_unlock();
    idle();
}

int task_cow(ptr_t addr) {
    // 复制错误页面
    addr = PAGE_ADDR(addr);
    struct task_struct *task = CUR_TCB;
    if (!task->mm) return -1;
    return vm_remap_page(addr, task->mm->pgdir);
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

UNUSED void test_thread() {
    // 测试内核线程
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