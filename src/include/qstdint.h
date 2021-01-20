//
// Created by pjs on 2021/1/5.
//

#ifndef QUARKOS_QSTDINT_H
#define QUARKOS_QSTDINT_H

#if defined(__i386__)
#include <stdint.h>
#else

// x86 指针长 32 位,万一以后写64位 OS 呢,233
#define POINTER_LENGTH 32

// 用于测试
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long int uint64_t;

#endif

#endif //QUARKOS_QSTDINT_H
