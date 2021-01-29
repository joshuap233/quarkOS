//
// Created by pjs on 2021/1/15.
//

#ifndef QUARKOS_QLIB_H
#define QUARKOS_QLIB_H

#include <stddef.h> // size_t and NULL
#include <stdbool.h>
#include <stdarg.h>
#include "stdint.h"
#include "vga.h"
#include "x86.h"

#ifdef __i386__
#define assertk(condition) {\
    if (!(condition)) {     \
        printfk("\nassert error: %s: %s: %u\n",__FILE__,__FUNCTION__,__LINE__); \
        panic();                        \
    }\
}

#else
#include <assert.h>
#define assertk(condition) assert(condition);
#endif


#ifdef __i386__

void printfk(char *__restrict str, ...);

#else
#include <stdio.h>
#define printfk printf
#endif

// 生成掩码
#define BIT_MASK(__type__, n) ((sizeof(__type__)*8==n)? \
        ((__type__)-1):\
        (((__type__)1<<n)-1))

#define MASK_U32(n) BIT_MASK(uint32_t, n)

// bit 为 0-7
static inline void set_bit(uint8_t *value, uint8_t bit) {
    *value |= (0b1 << bit);
}

// bit 为 0-7
static inline void clear_bit(uint8_t *value, uint8_t bit) {
    *value &= (~(0b1 << bit));
}

#endif //QUARKOS_QLIB_H
