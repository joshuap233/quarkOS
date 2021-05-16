//
// Created by pjs on 2021/1/15.
//

#ifndef QUARKOS_LIB_QLIB_H
#define QUARKOS_LIB_QLIB_H

#include "types.h"
#include <stdarg.h>
#include "drivers/vga.h"
#include "x86.h"
#include "drivers/pit.h"

u8_t log2(uint16_t val);

#ifdef __i386__
#ifdef DEBUG
#define assertk(condition) {\
    if (!(condition)) {     \
        printfk("\nassert error: %s: %s: %u\n",__FILE__,__FUNCTION__,__LINE__); \
        panic();                        \
    }\
}
#else

#define assertk(condition) {}

#endif // DEBUG

void printfk(char *__restrict str, ...);

#else

#include <assert.h>
#include <stdio.h>

#define assertk(condition) assert(condition);
#define printfk printf

#endif //__i386__


// 生成掩码
#define BIT_MASK(__type__, n) ((sizeof(__type__)*8==(n))? \
        ((__type__)-1):\
        (((__type__)1<<(n))-1))

#define MASK_U32(n) BIT_MASK(uint32_t, n)
#define MASK_U16(n) BIT_MASK(uint16_t, n)
#define MASK_U8(n)  BIT_MASK(uint8_t, n)

#define MIN(a, b) ((b)<(a)? (b):(a))
#define MAX(a, b) ((b)>(a)? (b):(a))

// 向上取整除法,可能溢出, y 为被除数
#define DIV_CEIL(x, y)  (((x) + (y) - 1) / (y))

// bit 为 0-7
INLINE void set_bit(uint8_t *value, uint8_t bit) {
    *value |= (0b1 << bit);
}

// bit 为 0-7
INLINE void clear_bit(uint8_t *value, uint8_t bit) {
    *value &= (~(0b1 << bit));
}

// bit 从 0 开始
#define CLEAR_BIT(value, bit) ((value) = (value) & (~((typeof(value))0b1 << (bit))))
#define SET_BIT(value, bit)   ((value) = (value) | ((typeof(value))0b1 << (bit)))

#define OR(a, b)  ((a)?(a):(b))

char *cur_func_name(ptr_t addr);

void stack_trace();

void panic();

void prints(char *str, size_t size);

#define container_of(ptr, type, member) \
    ((type *)((void *)(ptr) - offsetof(type,member)))

#ifdef TEST
#define test_start   printfk("test start: %s\n",__FUNCTION__);
#define test_pass    printfk("test pass : %s: %s: %u\n",__FILE__,__FUNCTION__,__LINE__);
#endif // TEST

#endif //QUARKOS_LIB_QLIB_H
