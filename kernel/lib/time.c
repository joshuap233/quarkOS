//
// Created by pjs on 2021/5/8.
//

#include "lib/time.h"


static u8_t month_days[] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

u64_t utc2stamp(struct cmos_time *time) {
    u64_t timestamp;
    u32_t days = 0;
    u32_t years = time->year - 1970;
    u32_t leap_year = ((time->year - 1968) >> 2) - (time->year - 1900) / 100 + (time->year - 1600) / 400;

    days += leap_year * 366 + (years - leap_year) * 365;

    bool is_leap = ((time->year % 100) && !(time->year & 3)) || !(time->year % 400);
    month_days[1] = is_leap ? 29 : 28;
    for (u8_t mon = 0; mon < time->month - 1; ++mon) {
        days += month_days[mon];
    }

    days += time->day - 1;
    timestamp = days * 24 * 60 * 60 + time->hour * 60 * 60 + time->minute * 60 + time->second;
    return timestamp;
}
