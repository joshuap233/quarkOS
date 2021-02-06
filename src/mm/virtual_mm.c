//
// Created by pjs on 2021/2/1.
//
//虚拟内存管理
#include "virtual_mm.h"
#include "types.h"
#include "mm.h"
#include "physical_mm.h"
#include "link_list.h"
#include "qstring.h"

//TODO: 使用伙伴管理空闲内存而不是链表

//修改/访问页目录: k_pde[index] = xxx
//修改/访问页表项: pointer_t *pte = PTE_ADDR(pde_index); pte[index]=xxx
static pde_t _Alignas(PAGE_SIZE) k_pde[] = {[0 ...PAGE_SIZE - 1]=(VM_KR | VM_NPRES)};
//内核虚拟页目录

static list_t free_list = {
        .addr=0,
        .end_addr = PHYMM - 1,
        .size = PHYMM,
        .next = MM_NULL
};


// 三个参数分别为:
// 需要映射的虚拟地址,物理地址, 需要映射的内存大小, (地址需要包括20位地址与12位标志)
// 映射成功返回 true
static bool mm_map(pointer_t va, pointer_t pa, uint32_t size) {
    size = SIZE_ALIGN(size);
    uint32_t pdeI = PDE_INDEX(va);
    uint32_t pteI = PTE_INDEX(va);

    for (; size > 0 && pdeI < N_PDE - 1; pdeI++, pteI = 0) {
        pde_t *pde = &k_pde[pdeI];
        pte_t *pte = (pte_t *) PTE_ADDR(pdeI); //页目录首地址(虚拟)
        if (!(*pde & VM_PRES)) {
            // 映射页表,phy_addr 为页表物理地址
            uint32_t phy_addr = phymm_alloc() | VM_KW | VM_PRES;
            if (!mm_map((pointer_t) pte, phy_addr, PAGE_SIZE)) {
                phymm_free(phy_addr);
                return false;
            };
            *pde = phy_addr;
            q_memset(pte, 0, PAGE_SIZE);
        }
        for (; pteI < N_PTE && size > 0; pteI++) {
            pte[pteI] = pa;
            size -= PAGE_SIZE;
            pa += PAGE_SIZE;
        }
    }
    return true;
}


// 初始化内核页表
void vmm_init() {
    uint32_t size = SIZE_ALIGN(K_END);
    cr3_t cr3 = CR3_CTRL | ((pointer_t) &k_pde);
    assertk(list_split(&free_list, 0, size));
    assertk(list_split(&free_list, PTE_VA, ENTRY_SIZE));
    // 页目录最后一项映射到 pde 首地址(即内核结束地址)
    k_pde[N_PDE - 1] = size | VM_KW | VM_PRES;

    //映射内核空间与低于 1M 的空间
    uint32_t phy_addr = 0;
    for (uint32_t i = 0; i < N_PDE - 1 && size > 0; ++i) {
        pte_t *pte = (pte_t *) phymm_alloc();
        q_memset(pte, 0, PAGE_SIZE);
        for (uint32_t j = 0; j < N_PTE && size > 0; ++j) {
            pte[j] = phy_addr | VM_PRES | VM_KW;
            phy_addr += PAGE_SIZE;
            size -= PAGE_SIZE;
        }
        k_pde[i] = ((pde_t) pte) | VM_KW | VM_PRES;
    }

    cr3_set(cr3);
}


// size 为 PAGE_SIZE 的整数倍
bool vmm_free(pointer_t va, uint32_t size) {
    uint32_t pdeI = PDE_INDEX(va);
    uint32_t pteI = PTE_INDEX(va);
    for (; size > 0 && pdeI < N_PDE; pdeI++, pteI = 0) {
        uint32_t start = pteI;
        pte_t *pte = (pte_t *) PTE_ADDR(pdeI);
        for (; pteI < N_PTE; pteI++, size -= PAGE_SIZE) {
            pte[pteI] = 0;
            //释放空页目录项对应的物理地址
            phymm_free(pte[pteI]);
        }
        //释放空页表
        if (start == 0 && pteI == N_PTE - 1) {
            phymm_free(k_pde[pdeI]);
            k_pde[pdeI] = VM_NPRES;
        }
    }
    list_free(&free_list, va, size);
}


// size 为需要分配的虚拟内存内存大小
void *vmm_alloc(uint32_t size) {
    size = SIZE_ALIGN(size);
    void *addr = list_split_ff(&free_list, size);
    if (addr == MM_NULL)return MM_NULL;
    pointer_t phy_addr = phymm_alloc();
    if (!mm_map((pointer_t) addr, phy_addr | VM_KW | VM_PRES, size)) {
        phymm_free(phy_addr);
        list_free(&free_list, (pointer_t) addr, size);
        return MM_NULL;
    };
    return addr;
}