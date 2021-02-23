//
// Created by pjs on 2021/2/23.
//

#include <stddef.h>
#include "types.h"
#include "sched/timer.h"
#include "sched/kthread.h"
#include "drivers/timer.h"


timer_t timer_[TIMER_COUNT];

//用于管理计时器
static struct timer_pool {
    timer_t *timer[TIMER_COUNT];
    timer_t *header; //指向timer链表第一个节点
    size_t top; //指向栈顶空元素
    size_t size;
} timer_pool;

void thread_timer_init() {
    timer_pool.top = 0;
    timer_pool.size = TIMER_COUNT;
    timer_pool.header = NULL;
    for (int i = 0; i < TIMER_COUNT; ++i) {
        timer_pool.timer[i] = &timer_[i];
    }
}

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

bool ms_sleep_until(uint64_t msc) {
    timer_t *t = alloc_timer();
    if (t == NULL) return false;
    t->time = msc;
    t->next = timer_pool.header;
    t->prev = NULL;
    t->thread = cur_task;
    if (timer_pool.header != NULL) {
        timer_pool.header->prev = t;
    }
    timer_pool.header = t;
    block_thread();
    return true;
}

bool ms_sleep(mseconds_t msc) {
    return ms_sleep_until(TIME_SINCE_BOOT + msc);
}

void timer_handle() {
    timer_t *header = timer_pool.header;
    while (header != NULL) {
        if (header->time >= TIME_SINCE_BOOT) {
            if (header->prev != NULL)
                header->prev->next = header->next;
            else
                timer_pool.header = header->next;
            unblock_thread(header->thread);
            // free_timer 不会销毁 header
            free_timer(header);
        }
        header = header->next;
    }
}
