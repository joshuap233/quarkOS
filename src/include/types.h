//
// Created by pjs on 2021/1/5.
//

#ifndef QUARKOS_TYPES_H
#define QUARKOS_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> // size_t and NULL

// ptr_t 存地址值, 万一以后写64位 OS 呢,233
typedef uint32_t ptr_t;


typedef uint32_t useconds_t;
typedef uint32_t mseconds_t; //毫秒

typedef uint8_t u8_t;
typedef uint16_t u16_t;
typedef uint32_t u32_t;
typedef uint64_t u64_t;

typedef uint32_t lba28;
#define K                   1024
#define M                   0x100000
#define G                   ((uint64_t)0x40000000)

#define PACKED __attribute__((packed))
#define INLINE __attribute__((always_inline)) static inline
#define INT    __attribute__((interrupt)) static void
#define UNUSED __attribute__((unused))

#define TEST

#endif //QUARKOS_TYPES_H
