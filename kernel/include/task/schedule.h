//
// Created by pjs on 2021/4/20.
//

#ifndef QUARKOS_TASK_SCHEDULE_H
#define QUARKOS_TASK_SCHEDULE_H

#include <types.h>
#include <lib/list.h>
#include <lib/spinlock.h>

#define TIME_SLICE_LENGTH 100  // 时间片长度为 100ms

void scheduler_init();

void schedule();

void sched_task_add(list_head_t *task);


#endif //QUARKOS_TASK_SCHEDULE_H
