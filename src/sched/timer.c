//
// Created by pjs on 2021/2/23.
//

#include <stddef.h>
#include "types.h"
#include "sched/timer.h"
#include "sched/kthread.h"
#include "drivers/timer.h"

// 当前线程剩余时间片
volatile uint64_t time_slice = 0;

static void _list_header_init(timer_t *header) {
    header->next = header;
    header->prev = header;
}

__attribute__((always_inline))
static inline void _list_add(timer_t *new, timer_t *prev, timer_t *next) {
    new->prev = prev;
    new->next = next;
    next->prev = new;
    prev->next = new;
}

__attribute__((always_inline))
static inline void _list_add_prev(timer_t *new, timer_t *target) {
    _list_add(new, target->prev, target);
}


__attribute__((always_inline))
static inline void _list_del(timer_t *list) {
    list->prev->next = list->next;
    list->next->prev = list->prev;
}


static timer_t _timer[TIMER_COUNT];

//用于管理计时器
static struct timer_pool {
    timer_t *timer[TIMER_COUNT];
    timer_t header; //头结点,始终为空
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
    _list_header_init(&timer_pool.header);
    for (int i = 0; i < TIMER_COUNT; ++i) {
        timer_pool.timer[i] = &_timer[i];
    }
}

bool ms_sleep_until(uint64_t msc) {
    timer_t *t = alloc_timer();
    if (t == NULL) return false;
    t->time = msc;
    t->thread = CUR_TCB;

    _list_add_prev(t, &timer_pool.header);
    block_thread();
    return true;
}

bool ms_sleep(mseconds_t msc) {
    return ms_sleep_until(TIME_SINCE_BOOT + msc);
}

void timer_handle() {
    for (timer_t *hdr = timer_pool.header.next, *next = hdr->next;
         hdr != &timer_pool.header; hdr = next, next = next->next) {
        if (hdr->time >= TIME_SINCE_BOOT) {
            _list_del(hdr);
            unblock_thread(hdr->thread);
            free_timer(hdr);
        }
    }
}
