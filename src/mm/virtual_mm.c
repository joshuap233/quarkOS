//
// Created by pjs on 2021/2/1.
//
//虚拟内存管理
#include "mm/virtual_mm.h"
#include "types.h"
#include "mm/mm.h"
#include "mm/physical_mm.h"
#include "mm/free_list.h"
#include "klib/qstring.h"
#include "multiboot2.h"
#include "mm/heap.h"

//刷新 tlb 缓存
static inline void tlb_flush(pointer_t va) {
    __asm__ volatile ("invlpg (%0)" : : "a" (va));
}


static page_dir_t _Alignas(PAGE_SIZE) pageDir = {
        .entry = {[0 ...PAGE_ENTRY_NUM - 1]=(VM_KR | VM_NPRES)}
};

//遍历页目录
#define for_each_pd(va, size) \
    for (uint32_t pdeI = PDE_INDEX(va), pteI = PTE_INDEX(va) ; (size) > 0 && pdeI < N_PDE - 1; pdeI++, pteI = 0)

//遍历页表
#define for_each_pt(pteI, size) for (; (pteI) < N_PTE && (size) > 0; (pteI)++,(size) -= PAGE_SIZE)


// 用于直接线性映射,未开启分页时调用
// 三个参数分别为: 需要映射的虚拟地址,物理地址, 需要映射的内存大小
void vmm_map(pointer_t va, pointer_t pa, uint32_t size, uint32_t flags) {
    assertk((va & ALIGN_MASK) == 0);

    for_each_pd(va, size) {
        pde_t *pde = &pageDir.entry[pdeI];
        //没有开启分页,使用物理地址访问页表
        page_table_t *pt = (page_table_t *) PAGE_ADDR(*pde);

        if (!(*pde & VM_PRES)) {
            *pde = phymm_alloc() | flags;
            pt = (page_table_t *) PAGE_ADDR(*pde);
            q_memset(pt, 0, sizeof(page_table_t));
        }

        for_each_pt(pteI, size) {
            pt->entry[pteI] = pa | flags;
            pa += PAGE_SIZE;

            tlb_flush(va);
            va += PAGE_SIZE;
        }
    }
}

// 映射虚拟地址 va ~ va+size-1
// 开启分页后才能使用
void vmm_mapv(pointer_t va, uint32_t size, uint32_t flags) {
    assertk((va & ALIGN_MASK) == 0);

    for_each_pd(va, size) {
        pde_t *pde = &pageDir.entry[pdeI];
        //已经开启分页,使用虚拟地址访问页表
        page_table_t *pt = (page_table_t *) PTE_ADDR(pdeI);

        if (!(*pde & VM_PRES)) {
            *pde = phymm_alloc() | flags;
            q_memset(pt, 0, sizeof(page_table_t));
        }

        for_each_pt(pteI, size) {
            pt->entry[pteI] = phymm_alloc() | flags;

            tlb_flush(va);
            va += PAGE_SIZE;
        }
    }
}

// 初始化内核页表
void vmm_init() {
    cr3_t cr3 = CR3_CTRL | ((pointer_t) &pageDir);
    free_list_init(g_vmm_start);
    //留出页表与堆的虚拟内存
    assertk(list_split(PTE_VA, PT_SIZE));
    assertk(list_split(HEAP_START, HEAP_SIZE));
    pageDir.entry[N_PDE - 1] = (pointer_t) &pageDir | VM_KW | VM_PRES;

    // _vmm_start 以下部分直接映射
    vmm_map(0, 0, SIZE_ALIGN(g_vmm_start), VM_KW | VM_PRES);
    test_vmm_map();
    cr3_set(cr3);
    test_vmm_map2();
    test_vmm_mapv();
}

// 开启分页后才能使用, size 为 PAGE_SIZE 的整数倍
// 不会释放空闲列表的虚拟内存
void vmm_unmap(void *va, uint32_t size) {
    assertk((size & ALIGN_MASK) == 0);
    for_each_pd((pointer_t) va, size) {
        uint32_t start = pteI;
        page_table_t *pt = (page_table_t *) PTE_ADDR(pdeI);

        for_each_pt(pteI, size) {
            phymm_free(pt->entry[pteI]);
            pt->entry[pteI] = VM_NPRES;

            tlb_flush((pointer_t)va);
            va += PAGE_SIZE;
        }

        //释放空页表物理内存,保留虚拟内存
        if (start == 0 && pteI == N_PTE - 1) {
            phymm_free(pageDir.entry[pdeI]);
            pageDir.entry[pdeI] = VM_NPRES;
        }
    }
}



// ======================== 测试 =====================

static uint16_t *test_pa;
static uint16_t *test_va;

// 需要在开启分页前调用
void test_vmm_map() {
    test_start;
    //测试直接映射函数
    test_pa = (uint16_t *) phymm_alloc();
    for (uint64_t i = 0; i < PAGE_SIZE / sizeof(uint16_t); ++i) {
        test_pa[i] = i;
    }
    test_va = list_split_ff(PAGE_SIZE);
    vmm_map((pointer_t) test_va, (pointer_t) test_pa, PAGE_SIZE, VM_KW | VM_PRES);
    test_pass;
}

// 开启分页后调用
void test_vmm_map2() {
    test_start;
    for (uint64_t i = 0; i < PAGE_SIZE / sizeof(uint16_t); ++i) {
        assertk(test_va[i] == i);
    }
    vmm_unmap((void *) test_va, PAGE_SIZE);
    list_free((pointer_t) test_va, PAGE_SIZE);
    test_pass;
}

void test_vmm_mapv() {
    test_start;
    test_va = list_split_ff(PAGE_SIZE);
    vmm_mapv((pointer_t) test_va, PAGE_SIZE, VM_KW | VM_PRES);
    for (uint64_t i = 0; i < PAGE_SIZE / sizeof(uint16_t); ++i) {
        test_va[i] = i;
    }
    vmm_unmap((void *) test_va, PAGE_SIZE);
    list_free((pointer_t) test_va, PAGE_SIZE);
    test_pass;
}