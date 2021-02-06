//
// Created by pjs on 2021/1/5.
//

#ifndef QUARKOS_TYPES_H
#define QUARKOS_TYPES_H


#include <stdint.h>
#include <stdbool.h>

typedef uint32_t pointer_t; //pointer_t 存地址值

// x86 指针长 32 位,万一以后写64位 OS 呢,233
#define POINTER_LENGTH 32

typedef uint32_t useconds_t;
typedef uint32_t mseconds_t; //毫秒

#endif //QUARKOS_TYPES_H
