//
// Created by pjs on 2021/2/21.
//

#ifndef QUARKOS_STACK_TRACE_H
#define QUARKOS_STACK_TRACE_H

#include "types.h"

char *cur_func_name(pointer_t addr);
void stack_trace();
#endif //QUARKOS_STACK_TRACE_H
