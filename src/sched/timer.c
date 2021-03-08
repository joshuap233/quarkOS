//
// Created by pjs on 2021/2/23.
//

#include <stddef.h>
#include "types.h"
#include "sched/timer.h"
#include "sched/kthread.h"
#include "drivers/timer.h"

#include "klib/list.h"

#define timer_entry(ptr) list_entry(ptr, timer_t, head)
#define HEAD timer_pool.header
// 当前线程剩余时间片
volatile uint64_t g_time_slice = 0;

static timer_t _timer[TIMER_COUNT];

//用于管理计时器
static struct timer_pool {
    timer_t *timer[TIMER_COUNT];
    list_head_t header; //头结点,始终为空
    size_t top; //指向栈顶空元素
    size_t size;
} timer_pool;

__attribute__((always_inline))
static inline timer_t *alloc_timer() {
    if (timer_pool.top != 0)
        return timer_pool.timer[--timer_pool.top];
    return NULL;
}

__attribute__((always_inline))
static inline void free_timer(timer_t *t) {
    if (timer_pool.top != timer_pool.size)
        timer_pool.timer[timer_pool.top++] = t;
}

void thread_timer_init() {
    timer_pool.top = 0;
    timer_pool.size = TIMER_COUNT;
    list_header_init(&HEAD);
    for (int i = 0; i < TIMER_COUNT; ++i) {
        timer_pool.timer[i] = &_timer[i];
    }
}

bool ms_sleep_until(uint64_t msc) {
    timer_t *t = alloc_timer();
    if (t == NULL) return false;
    t->time = msc;
    t->thread = CUR_TCB;

    list_add_prev(&t->head, &HEAD);
    block_thread();
    return true;
}

bool ms_sleep(mseconds_t msc) {
    return ms_sleep_until(G_TIME_SINCE_BOOT + msc);
}

void timer_handle() {
    list_for_each_del(&HEAD) {
        timer_t *tmp = timer_entry(hdr);
        if (tmp->time >= G_TIME_SINCE_BOOT) {
            list_del(hdr);
            unblock_thread(tmp->thread);
            free_timer(tmp);
        }
    }
}
