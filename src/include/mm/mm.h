//
// Created by pjs on 2021/2/1.
//

#ifndef QUARKOS_MM_H
#define QUARKOS_MM_H

#include "types.h"
#include "klib/qlib.h"
#include "mm/heap.h"
#include "mm/vmm.h"
#include "mm/pmm.h"

//页错误错误码
typedef struct pf_error_code {
    uint16_t p: 1;   //置 0 则异常由页不存在引起,否则由特权级保护引起
    uint16_t w: 1;   //置 0 则访问是读取
    uint16_t u: 1;   //置 0 则特权模式下发生的异常
    uint16_t r: 1;
    uint16_t i: 1;
    uint16_t pk: 1;
    uint16_t zero: 10;
    uint16_t sgx: 1;
    uint16_t zero1: 15;
}PACKED pf_error_code_t;

#endif //QUARKOS_MM_H
