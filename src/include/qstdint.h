//
// Created by pjs on 2021/1/5.
//

#ifndef QUARKOS_QSTDINT_H
#define QUARKOS_QSTDINT_H

#if defined(__i386__)

#include <stdint.h>
#include <stdbool.h>

typedef uint32_t pointer_t;

// x86 指针长 32 位,万一以后写64位 OS 呢,233
#define POINTER_LENGTH 32

//驱动程序常常需要检测状态: 0/1
typedef bool status_t;
#define ZERO  false
#define NZ    true

#else


// 用于测试
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long int uint64_t;
typedef uint32_t pointer_t;

#endif

#endif //QUARKOS_QSTDINT_H
