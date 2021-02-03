//
// Created by pjs on 2021/2/1.
//
//虚拟内存管理
#include "virtual_mm.h"
#include "types.h"
#include "mm.h"
#include "physical_mm.h"

typedef struct link_list {
    pointer_t addr;
    struct link_list *next;
} link_list_t;
static pde_t _Alignas(PAGE_SIZE) kernel_pde[N_PDE] = {0};

//void list_addend(link_list_t *list, pointer_t addr) {
//
//}
//
//void list_remove(link_list_t *list, pointer_t addr) {
//
//}

// 初始化内核页表
void vmm_init() {
    cr3_t cr3 = CR3_CTRL | ((pointer_t) &kernel_pde);
    uint32_t start = 0;
    //只映射内核空间与低于 1M 的空间
    uint32_t size = PAGE_ALIGN(K_END);
    for (int i = 0; i < N_PDE && size > 0; ++i) {
        pte_t *pte = (pte_t *) phymm_alloc();
        for (int j = 0; j < N_PTE && size > 0; ++j) {
            size -= PAGE_SIZE;
            pte[j] = start | MM_PRES | MM_KW;
            start += PAGE_SIZE;
        }

        kernel_pde[i] = ((pde_t) pte) | MM_KW | MM_PRES;
    }

    cr3_set(cr3);
}