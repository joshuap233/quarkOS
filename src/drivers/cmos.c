//
// Created by pjs on 2021/5/8.
//
// https://wiki.osdev.org/CMOS

#include "drivers/cmos.h"
#include "x86.h"
#include "lib/time.h"
#include "drivers/pit.h"

u64_t startup_timestamp = 0;

// 当前年份,如果没有世纪寄存器,则用于判断当前世纪
#define CURRENT_YEAR        2021

// 使用使用 apic 检测是否有世纪寄存器, cmos 年份只会显示后两位
int32_t century_register = 0x00;

enum {
    cmos_address = 0x70,
    cmos_data = 0x71
};

INLINE int32_t get_update_in_progress_flag() {
    outb(cmos_address, 0x0A);
    return (inb(cmos_data) & 0x80);
}

INLINE u8_t get_RTC_register(int32_t reg) {
    outb(cmos_address, reg);
    return inb(cmos_data);
}

#define CMOS_READ(t) { \
    while (get_update_in_progress_flag()); \
    (t)->second = get_RTC_register(0x00); \
    (t)->minute = get_RTC_register(0x02);\
    (t)->hour = get_RTC_register(0x04);\
    (t)->day = get_RTC_register(0x07);\
    (t)->month = get_RTC_register(0x08);\
    (t)->year = get_RTC_register(0x09);\
    if (century_register != 0) {\
    century = get_RTC_register(century_register);\
     }\
}

// 读取 utc 时间或者夏令时
void read_rtc(struct cmos_time *t) {
    u8_t century;
    u8_t last_second;
    u8_t last_minute;
    u8_t last_hour;
    u8_t last_day;
    u8_t last_month;
    u8_t last_year;
    u8_t last_century;
    u8_t registerB;


    // 确保 cmos 没有在更新
    CMOS_READ(t);

    // 多次读取,防止因为 cmos 正在更新导致时间误差
    do {
        last_second = t->second;
        last_minute = t->minute;
        last_hour = t->hour;
        last_day = t->day;
        last_month = t->month;
        last_year = t->year;
        last_century = century;

        CMOS_READ(t);
    } while ((last_second != t->second) || (last_minute != t->minute) || (last_hour != t->hour) ||
             (last_day != t->day) || (last_month != t->month) || (last_year != t->year) ||
             (last_century != century));

    registerB = get_RTC_register(0x0B);

    // 转换为 bcd 编码
    if (!(registerB & 0x04)) {
        t->second = (t->second & 0x0F) + ((t->second / 16) * 10);
        t->minute = (t->minute & 0x0F) + ((t->minute / 16) * 10);
        t->hour = ((t->hour & 0x0F) + (((t->hour & 0x70) / 16) * 10)) | (t->hour & 0x80);
        t->day = (t->day & 0x0F) + ((t->day / 16) * 10);
        t->month = (t->month & 0x0F) + ((t->month / 16) * 10);
        t->year = (t->year & 0x0F) + ((t->year / 16) * 10);
        if (century_register != 0) {
            century = (century & 0x0F) + ((century / 16) * 10);
        }
    }

    // 12 小时制转换为 24 小时制

    if (!(registerB & 0x02) && (t->hour & 0x80)) {
        t->hour = ((t->hour & 0x7F) + 12) % 24;
    }

    // 计算年份

    if (century_register != 0) {
        t->year += century * 100;
    } else {
        t->year += (CURRENT_YEAR / 100) * 100;
        if (t->year < CURRENT_YEAR) t->year += 100;
    }
}

void cmos_init() {
    struct cmos_time time;
    read_rtc(&time);
    startup_timestamp = utc2stamp(&time);
}

u64_t cur_timestamp() {
    return startup_timestamp + G_TIME_SINCE_BOOT * 100;
}