//
// Created by pjs on 2021/5/8.
//

#ifndef QUARKOS_LIB_TIME_H
#define QUARKOS_LIB_TIME_H
#include "drivers/cmos.h"
#include "types.h"

u64_t utc2stamp(struct cmos_time *time);

#endif //QUARKOS_LIB_TIME_H
