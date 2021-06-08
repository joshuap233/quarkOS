//
// Created by pjs on 2021/4/20.
//
// 多级反馈队列
// 线程运行次数用于 debug
#include <sched/schedule.h>
#include <sched/fork.h>
#include <sched/timer.h>
#include <drivers/pit.h>
#include <lib/qlib.h>
#include <lib/irlock.h>


extern void switch_to(context_t **cur_context, context_t *next_context);

static uint64_t update_priority_time; // 单位为 10 毫秒

static list_head_t *chose_next_task();

static void reset_priority();

#define RESET_PRIORITY_INTERVAL 2  // 2 秒重置一次线程优先级

static struct scheduler {
    queue_t queue[TASK_MAX_PRIORITY + 1];
} scheduler;

void scheduler_init() {
    for (int i = 0; i <= TASK_MAX_PRIORITY; ++i) {
        queue_init(&scheduler.queue[i]);
    }
    update_priority_time = G_TIME_SINCE_BOOT + RESET_PRIORITY_INTERVAL * 1000;
}

// 用户空间任务创建后,内核线程与用户任务都使用相同的 gs,ds,es,fs
// 特权级切换时,中断会修改保存 cs,ss
// 使用用户线程的页表
void schedule() {
    extern void set_tss_esp(void *stack);

    ir_lock_t lock;
    ir_lock(&lock);
    if (G_TIME_SINCE_BOOT >= update_priority_time) {
        reset_priority();
        update_priority_time = G_TIME_SINCE_BOOT + RESET_PRIORITY_INTERVAL * 1000;
    }

    tcb_t *cur_task = CUR_TCB, *next_task;
    list_head_t *next = chose_next_task();
    list_head_t *cur = &cur_task->run_list;
    if (next == idle_task && &CUR_HEAD == idle_task) {
        assertk(tcb_entry(idle_task)->timer_slice == 0);
        return;
    }
    // init task 时间片始终为 0,且不在反馈队列中
    if (cur != idle_task) {
        if (cur_task->priority != 0) {
            if (cur_task->timer_slice == 0) {
                cur_task->timer_slice = TIME_SLICE_LENGTH;
                cur_task->priority--;
            }
        } else {
            cur_task->timer_slice = TIME_SLICE_LENGTH;
        }
        if (cur_task->state == TASK_RUNNING) {
            sched_task_add(cur);
        }
    }
    next_task = tcb_entry(next);
    if (next_task->mm){
        // 设置用户任务内核栈
        set_tss_esp(next_task->stack + PAGE_SIZE);
    }
    switch_to(&cur_task->context, next_task->context);
    ir_unlock(&lock);
}


static list_head_t *chose_next_task() {
    // 会从队列删除需要运行的任务
    for (int i = TASK_MAX_PRIORITY; i >= 0; i--) {
        queue_t *queue = &scheduler.queue[i];
        if (!queue_empty(queue)) {
            return queue_get(queue);
        }
    }
    return idle_task;
}

// 将所有任务优先级提升到最高优先级
static void reset_priority() {
    list_head_t *hdr1 = NULL, *tail1 = NULL;
    list_head_t *hdr = &scheduler.queue[TASK_MAX_PRIORITY];
    list_head_t *tail = hdr->prev;
    // 插入 top 时,高优先级在低优先级前
    for (int i = TASK_MAX_PRIORITY - 1; i >= 0; i--) {
        queue_t *queue = &scheduler.queue[i];
        if (!list_empty(queue->next)) {
            hdr1 = queue->next;
            tail1 = queue->prev;

            hdr1->prev = tail;
            tail->next = hdr1;
            tail = tail1;
            list_header_init(queue);
        }
    }
    if (hdr1 != NULL) {
        assertk(tail1 != NULL);
        tail1->next = hdr;
        hdr->prev = tail1;
    }

    list_for_each_rev(hdr, &scheduler.queue[TASK_MAX_PRIORITY]) {
        if (tcb_entry(hdr)->priority == TASK_MAX_PRIORITY)
            break;
        tcb_entry(hdr)->priority = TASK_MAX_PRIORITY;
    }
}


void sched_task_add(list_head_t *task) {
    assertk(task);
    tcb_t *new = tcb_entry(task);
    queue_put(task, &scheduler.queue[new->priority]);
}
