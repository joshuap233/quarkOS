//
// Created by pjs on 2021/2/1.
//
//虚拟内存管理
#include "virtual_mm.h"
#include "types.h"
#include "mm.h"
#include "physical_mm.h"
#include "free_list.h"
#include "qstring.h"
#include "multiboot2.h"
#include "heap.h"

//TODO: 使用buddy/slab
//TODO: 添加测试
//修改/访问页目录方法: k_pde[i] = xxx;
//修改/访问页表方法:  pde_t *pte = PTE_ADDR(pde_index), pte 为页表索引对应的页目录首地址
static pde_t _Alignas(PAGE_SIZE) k_pde[] = {[0 ...PAGE_SIZE - 1]=(VM_KR | VM_NPRES)};


// 用于直接线性映射
// 三个参数分别为: 需要映射的虚拟地址,物理地址, 需要映射的内存大小
void vmm_map(pointer_t va, pointer_t pa, uint32_t size, uint32_t flags) {
    assertk((va & ALIGN_MASK) == 0);
    uint32_t pdeI = PDE_INDEX(va), pteI = PTE_INDEX(va);
    bool paging = is_paging();

    for (; size > 0 && pdeI < N_PDE - 1; pdeI++, pteI = 0) {
        pde_t *pde = &k_pde[pdeI];  //页目录项
        pte_t *pte = (pte_t *) PTE_ADDR(pdeI);  //页表首地址(虚拟)

        if (!(*pde & VM_PRES)) {
            *pde = phymm_alloc() | flags;
            //未启用分页时,使用物理地址访问 pte
            if (!paging) pte = (pte_t *) PAGE_ADDR(*pde);
            q_memset(pte, 0, PAGE_SIZE);
        }

        for (; pteI < N_PTE && size > 0; pteI++) {
            pte[pteI] = pa | flags;
            size -= PAGE_SIZE;
            pa += PAGE_SIZE;
        }
    }
}

// 初始化内核页表
void vmm_init() {
    cr3_t cr3 = CR3_CTRL | ((pointer_t) k_pde);
    free_list_init(SIZE_ALIGN(g_vmm_start));
    //留出页表与堆的虚拟内存
    assertk(list_split(PTE_VA, PT_SIZE));
    assertk(list_split(HEAP_START, HEAP_SIZE));
    k_pde[N_PDE - 1] = (pointer_t) k_pde | VM_KW | VM_PRES;

    // _vmm_start 以下部分直接映射
    vmm_map(0, 0, SIZE_ALIGN(g_vmm_start), VM_KW | VM_PRES);
    cr3_set(cr3);
}


// size 为 PAGE_SIZE 的整数倍
void vmm_unmap(void *va, uint32_t size) {
    assertk((size & ALIGN_MASK) == 0);
    uint32_t pdeI = PDE_INDEX((pointer_t) va), pteI = PTE_INDEX((pointer_t) va);
    for (; size > 0 && pdeI < N_PDE; pdeI++, pteI = 0) {
        uint32_t start = pteI;
        pte_t *pte = (pte_t *) PTE_ADDR(pdeI);
        for (; pteI < N_PTE; pteI++, size -= PAGE_SIZE) {
            pte[pteI] = 0;
            phymm_free(pte[pteI]);
        }
        //释放空页表
        if (start == 0 && pteI == N_PTE - 1) {
            phymm_free(k_pde[pdeI]);
            k_pde[pdeI] = VM_NPRES;
        }
    }
    list_free((pointer_t) va, size);
}


// 映射虚拟地址 va ~ va+size-1
// 开启分页后才能使用
void vmm_mapv(pointer_t va, uint32_t size, uint32_t flags) {
    assertk((va & ALIGN_MASK) == 0);
    uint32_t pdeI = PDE_INDEX(va), pteI = PTE_INDEX(va);

    for (; size > 0 && pdeI < N_PDE - 1; pdeI++, pteI = 0) {
        pde_t *pde = &k_pde[pdeI];
        pte_t *pte = (pte_t *) PTE_ADDR(pdeI);

        if (!(*pde & VM_PRES)) {
            *pde = phymm_alloc() | flags;
            q_memset(pte, 0, PAGE_SIZE);
        }

        for (; pteI < N_PTE && size > 0; pteI++) {
            pte[pteI] = phymm_alloc() | flags;
            size -= PAGE_SIZE;
        }
    }
}
