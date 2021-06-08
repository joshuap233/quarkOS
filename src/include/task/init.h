//
// Created by pjs on 2021/4/6.
//

#ifndef QUARKOS_TASK_INIT_H
#define QUARKOS_TASK_INIT_H

#include "types.h"

void task_init();
void scheduler_init();

#ifdef TEST
void test_thread();
#endif //TEST

#endif //QUARKOS_TASK_INIT_H
