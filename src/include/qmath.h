//
// Created by pjs on 2021/1/19.
//

#ifndef QUARKOS_QMATH_H
#define QUARKOS_QMATH_H

#include "types.h"

// 在整数过大的情况,q_ceilf 会调用 _q_ceilf
float q_ceilf(float _arg);

float _q_ceilf(float _arg);

float q_floorf(float _arg);

// unsigned 32 位除法,取 ceil, 第一个参数为被除数
uint32_t divUc(uint32_t dividend, uint32_t divider);

#endif //QUARKOS_QMATH_H
