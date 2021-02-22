//
// Created by pjs on 2021/2/22.
//

#ifndef QUARKOS_KLOCK_H
#define QUARKOS_KLOCK_H

#include "x86.h"

//使用这种方式会导致中断丢失,即无论调用 kLock 前是否开启中断
//调用 kRelease 后都会开启中断
extern void k_lock();

extern void k_lock_release();

#endif //QUARKOS_KLOCK_H
