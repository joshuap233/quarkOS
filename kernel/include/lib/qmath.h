//
// Created by pjs on 2021/1/19.
//

#ifndef QUARKOS_LIB_QMATH_H
#define QUARKOS_LIB_QMATH_H

#include <types.h>

// 在整数过大的情况,q_ceilf 会调用 _q_ceilf
float ceilf(float _arg);

float _ceilf(float _arg);

float floorf(float _arg);


#endif //QUARKOS_LIB_QMATH_H
