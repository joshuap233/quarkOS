//
// Created by pjs on 2021/5/8.
//

#ifndef QUARKOS_DRIVERS_CMOS_H
#define QUARKOS_DRIVERS_CMOS_H

#include "types.h"

struct cmos_time {
    u8_t second;
    u8_t minute;
    u8_t hour;
    u8_t day;
    u8_t month;
    int32_t year;
};

extern int32_t century_register;
// 系统启动时的时间戳
extern u64_t startup_timestamp;

void read_rtc();
u64_t cur_timestamp();

#endif //QUARKOS_DRIVERS_CMOS_H
