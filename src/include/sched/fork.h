//
// Created by pjs on 2021/6/3.
//

#ifndef QUARKOS_SCHED_FORK_H
#define QUARKOS_SCHED_FORK_H

#include <sched/task.h>


void task_set_name(pid_t pid, const char *name);

int8_t task_sleep(list_head_t *block_list, spinlock_t *lk);

void task_wakeup(list_head_t *task);

list_head_t *task_get_run_list(pid_t pid);

extern list_head_t *init_task;

int kthread_create(kthread_t *tid, void *(worker)(void *), void *args);

void task_set_time_slice(pid_t pid, u16_t time_slice);

void task_exit();


_Noreturn INLINE void idle() {
    while (1) {
        halt();
    }
}


#endif //QUARKOS_SCHED_FORK_H
