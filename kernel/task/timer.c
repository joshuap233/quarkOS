//
// Created by pjs on 2021/2/23.
//

#include <types.h>
#include <drivers/pit.h>
#include <isr.h>
#include <lib/qlib.h>
#include <lib/list.h>
#include <task/schedule.h>
#include <task/fork.h>
#include <task/timer.h>
#include <drivers/lapic.h>


#define timer_entry(ptr) list_entry(ptr, timer_t, head)
#define P_HEAD timer_pool.header

static timer_t _timer[TIMER_COUNT];

INT clock_isr(interrupt_frame_t *frame);


//用于管理计时器
static struct timer_pool {
    timer_t *timer[TIMER_COUNT];
    list_head_t header; //头结点,始终为空
    size_t top; //指向栈顶空元素
    size_t size;
} timer_pool;

static timer_t *alloc_timer() {
    if (timer_pool.top != 0)
        return timer_pool.timer[--timer_pool.top];
    return NULL;
}

static void free_timer(timer_t *t) {
    if (timer_pool.top != timer_pool.size)
        timer_pool.timer[timer_pool.top++] = t;
}

void thread_timer_init() {
    timer_pool.top = TIMER_COUNT;
    timer_pool.size = TIMER_COUNT;
    list_header_init(&P_HEAD);
    for (int i = 0; i < TIMER_COUNT; ++i) {
        timer_pool.timer[i] = &_timer[i];
    }

    reg_isr(IRQ0, clock_isr);
}

bool ms_sleep_until(uint64_t msc) {
    timer_t *t = alloc_timer();
    if (t == NULL) return false;
    t->time = msc;
    t->thread = &CUR_TCB->run_list;
    list_add_prev(&t->head, &P_HEAD);
    task_sleep(NULL, NULL);
    return true;
}

bool ms_sleep(mseconds_t msc) {
    return ms_sleep_until(G_TIME_SINCE_BOOT + msc);
}


// PIC 0 号中断,PIT 时钟中断
INT clock_isr(UNUSED interrupt_frame_t *frame) {
    G_TIME_SINCE_BOOT += 10;

    list_head_t *hdr = (&P_HEAD)->next;
    while (hdr != &P_HEAD) {
        timer_t *tmp = timer_entry(hdr);
        hdr = hdr->next;
        if (tmp->time <= G_TIME_SINCE_BOOT) {
            task_wakeup(tmp->thread);
            list_del(&tmp->head);
            free_timer(tmp);
        }
    }
    lapicEoi();

    tcb_t *task = CUR_TCB;
    if (task->timer_slice == 0) {
        schedule();
    } else {
        assertk(&task->run_list != getCpu()->idle);
        task->timer_slice -= 10;
    }
}
