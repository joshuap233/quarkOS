//
// Created by pjs on 2021/5/8.
//

#ifndef QUARKOS_DRIVERS_CMOS_H
#define QUARKOS_DRIVERS_CMOS_H

#include <types.h>

struct cmos_time {
    u8_t second;
    u8_t minute;
    u8_t hour;
    u8_t day;
    u8_t month;
    int32_t year;
};

// 系统启动时的时间戳
extern u64_t startup_timestamp;

void cmos_w(u16_t addr, u8_t data);
u64_t cur_timestamp();

#endif //QUARKOS_DRIVERS_CMOS_H
