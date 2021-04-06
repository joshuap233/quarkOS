//
// Created by pjs on 2021/3/11.
//

#include "sched/user.h"
//#define U_PD_INDEX 767
//#define U_PTE_VA        ((pointer_t)U_PD_INDEX << 22) //PDE 虚拟地址
//#define PTE_ADDR(pde_index) (U_PTE_VA + (pde_index)*PAGE_SIZE)


int user_test() {
    int i = 1 + 2;
    return i;
}

// 复制内核页表
void cpy_pd() {

}