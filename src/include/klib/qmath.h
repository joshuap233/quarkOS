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


#endif //QUARKOS_KLIB_QMATH_H
