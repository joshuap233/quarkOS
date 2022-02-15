//
// Created by pjs on 2021/6/3.
//

#ifndef QUARKOS_TASK_FORK_H
#define QUARKOS_TASK_FORK_H

#include <task/task.h>

#define CLONE_KERNEL_STACK      0b1
#define CLONE_MM               0b10
#define CLONE_FILE            0b100

void task_set_name(pid_t pid, const char *name);

int8_t task_sleep(list_head_t *block_list, spinlock_t *lk);

void task_wakeup(list_head_t *task);

list_head_t *task_get_run_list(pid_t pid);

int kthread_create(kthread_t *tid, void *(worker)(void *), void *args);

void task_set_time_slice(pid_t pid, u16_t time_slice);

void task_exit();

void user_task_init();

pid_t kernel_fork();

_Noreturn INLINE void idle() {
    while (1) {
        halt();
    }
}


#endif //QUARKOS_TASK_FORK_H
