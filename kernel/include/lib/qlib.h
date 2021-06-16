//
// Created by pjs on 2021/1/15.
//

#ifndef QUARKOS_LIB_QLIB_H
#define QUARKOS_LIB_QLIB_H

#include <types.h>
#include <terminal.h>

u8_t log2(uint16_t val);

// 生成掩码
#define BIT_MASK(__type__, n) ((sizeof(__type__)*8==(n))? \
        ((__type__)-1):\
        (((__type__)1<<(n))-1))

#define MASK_U64(n) BIT_MASK(uint64_t, n)
#define MASK_U32(n) BIT_MASK(uint32_t, n)
#define MASK_U16(n) BIT_MASK(uint16_t, n)
#define MASK_U8(n)  BIT_MASK(uint8_t, n)

#define MIN(a, b) ((b)<(a)? (b):(a))
#define MAX(a, b) ((b)>(a)? (b):(a))

// 向上取整除法,可能溢出, y 为被除数
#define DIV_CEIL(x, y)  (((x) + (y) - 1) / (y))

#define IS_POWER_OF_2(x) (!((x)&((x)-1)))

// 将不是 2 的幂的数变成 2 的幂
INLINE u32_t fixSize(u32_t size) {
    if (IS_POWER_OF_2(size))
        return size;
    size |= size >> 1;
    size |= size >> 2;
    size |= size >> 4;
    size |= size >> 8;
    size |= size >> 16;
    return size + 1;
}

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

#define container_of(ptr, type, member) \
    ((type *)((void *)(ptr) - offsetof(type,member)))

#endif //QUARKOS_LIB_QLIB_H
