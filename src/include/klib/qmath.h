//
// Created by pjs on 2021/1/19.
//

#ifndef QUARKOS_KLIB_QMATH_H
#define QUARKOS_KLIB_QMATH_H

#include "types.h"

// 在整数过大的情况,q_ceilf 会调用 _q_ceilf
float q_ceilf(float _arg);

float _q_ceilf(float _arg);

float q_floorf(float _arg);

// 向上取整除法,可能溢出, y 为被除数
#define DIV_CEIL(x, y)  (((x) + (y) - 1) / (y))

#endif //QUARKOS_KLIB_QMATH_H
