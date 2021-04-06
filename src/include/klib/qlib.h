//
// Created by pjs on 2021/1/15.
//

#ifndef QUARKOS_QLIB_H
#define QUARKOS_QLIB_H

#include <stdbool.h>
#include <stdarg.h>
#include "types.h"
#include "drivers/vga.h"
#include "x86.h"
#include "drivers/timer.h"

#ifdef __i386__

#define assertk(condition) {\
    if (!(condition)) {     \
        printfk("\nassert error: %s: %s: %u\n",__FILE__,__FUNCTION__,__LINE__); \
        panic();                        \
    }\
}

void printfk(char *__restrict str, ...);

#else

#include <assert.h>
#include <stdio.h>

#define assertk(condition) assert(condition);
#define printfk printf

#endif


// 生成掩码
#define BIT_MASK(__type__, n) ((sizeof(__type__)*8==(n))? \
        ((__type__)-1):\
        (((__type__)1<<(n))-1))

#define MASK_U32(n) BIT_MASK(uint32_t, n)
#define MASK_U16(n) BIT_MASK(uint16_t, n)
#define MASK_U8(n)  BIT_MASK(uint8_t, n)

// bit 为 0-7
INLINE void set_bit(uint8_t *value, uint8_t bit) {
    *value |= (0b1 << bit);
}

// bit 为 0-7
INLINE void clear_bit(uint8_t *value, uint8_t bit) {
    *value &= (~(0b1 << bit));
}

// bit 从 0 开始
#define CLEAR_BIT(value, bit) ((value) & (~((typeof(value))0b1 << (bit))))
#define SET_BIT(value, bit)   ((value) | ((typeof(value))0b1 << (bit)))

char *cur_func_name(pointer_t addr);

void stack_trace();

void panic();


#define test_start   printfk("test start: %s\n",__FUNCTION__);
#define test_pass    printfk("test pass : %s: %s: %u\n",__FILE__,__FUNCTION__,__LINE__);


#endif //QUARKOS_QLIB_H
