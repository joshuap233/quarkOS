//
// Created by pjs on 2021/2/1.
//
//虚拟内存管理
#include "types.h"
#include "mm/vmm.h"
#include "mm/pmm.h"
#include "mm/heap.h"

#include "mm/free_list.h"
#include "klib/qstring.h"
#include "multiboot2.h"
#include "x86.h"

//内核页表本身在页表内的索引
#define K_PD_INDEX        1023
//PDE 虚拟地址
#define K_PTE_VA         ((pointer_t)K_PD_INDEX << 22) \
//返回页目录索引为index的页表项首地址
#define PTE_ADDR(pde_index) (K_PTE_VA + (pde_index)*PAGE_SIZE)

extern void cr3_set(pointer_t);

static pdr_t _Alignas(PAGE_SIZE) pageDir = {
        .entry = {[0 ...PAGE_ENTRY_NUM - 1]=(VM_KR | VM_NPRES)}
};

// pde 为页目录自身映射到的页目录项
//ptb_t *get_ptb(pointer_t va, uint32_t flags, pde_t *pde, bool paging) {
//
//
////    if (!(*pde & VM_PRES)) {
////        *pde = phymm_alloc() | flags;
////        pt = (ptb_t *) PAGE_ADDR(*pde);
////        q_memset(pt, 0, sizeof(ptb_t));
////    }
//}

// 用于直接线性映射,未开启分页时调用
// 三个参数分别为: 需要映射的虚拟地址,物理地址, 需要映射的内存大小
void vmm_map(pointer_t va, pointer_t pa, uint32_t size, uint32_t flags) {
    assertk((va & ALIGN_MASK) == 0);

    for (pointer_t end = va + size; va < end;) {
        pde_t *pde = &pageDir.entry[PDE_INDEX(va)];
        //没有开启分页,使用物理地址访问页表
        ptb_t *pt = (ptb_t *) PAGE_ADDR(*pde);

        if (!(*pde & VM_PRES)) {
            *pde = phymm_alloc() | flags;
            pt = (ptb_t *) PAGE_ADDR(*pde);
            q_memset(pt, 0, sizeof(ptb_t));
        }

        for (; PTE_INDEX(va) < N_PTE && va < end; va += PAGE_SIZE) {
            pt->entry[PTE_INDEX(va)] = pa | flags;
            pa += PAGE_SIZE;

            tlb_flush(va);
        }
    }
}

// 映射虚拟地址 va ~ va+size-1
// 开启分页后才能使用
void vmm_mapv(pointer_t va, uint32_t size, uint32_t flags) {
    assertk((va & ALIGN_MASK) == 0);

    for (pointer_t end = va + size; va < end;) {
        pde_t *pde = &pageDir.entry[PDE_INDEX(va)];
        //已经开启分页,使用虚拟地址访问页表
        ptb_t *pt = (ptb_t *) PTE_ADDR(PDE_INDEX(va));

        if (!(*pde & VM_PRES)) {
            *pde = phymm_alloc() | flags;
            q_memset(pt, 0, sizeof(ptb_t));
        }

        for (; PTE_INDEX(va) < N_PTE && va < end; va += PAGE_SIZE) {
            pt->entry[PTE_INDEX(va)] = phymm_alloc() | flags;

            tlb_flush(va);

        }
    }
}

// 初始化内核页表
void vmm_init() {
    cr3_t cr3 = CR3_CTRL | ((pointer_t) &pageDir);
    free_list_init(g_vmm_start);
    //留出页表与堆的虚拟内存
    assertk(list_split(K_PTE_VA, PT_SIZE));
    assertk(list_split(HEAP_START, HEAP_SIZE));
    pageDir.entry[N_PDE - 1] = (pointer_t) &pageDir | VM_KW | VM_PRES;

    // _vmm_start 以下部分直接映射
    vmm_map(0, 0, SIZE_ALIGN(g_vmm_start), VM_KW | VM_PRES);

#ifdef TEST
    test_vmm_map();
#endif //TEST

    cr3_set(cr3);

#ifdef TEST
    test_vmm_map2();
    test_vmm_mapv();
#endif //TEST
}

// 开启分页后才能使用, size 为 PAGE_SIZE 的整数倍
// 不会释放空闲列表的虚拟内存
void vmm_unmap(void *_va, uint32_t size) {
    assertk((size & ALIGN_MASK) == 0);
    pointer_t va = (pointer_t) _va;
    for (pointer_t end = va + size; va < end;) {
        uint32_t start = PTE_INDEX(va);
        ptb_t *pt = (ptb_t *) PTE_ADDR(PDE_INDEX(va));

        for (; PTE_INDEX(va) < N_PTE && va < end; va += PAGE_SIZE) {
            phymm_free(pt->entry[PTE_INDEX(va)]);
            pt->entry[PTE_INDEX(va)] = VM_NPRES;

            tlb_flush((pointer_t) va);
        }

        //释放空页表物理内存,保留虚拟内存
        if (start == 0 && PTE_INDEX(va) == N_PTE - 1) {
            phymm_free(pageDir.entry[PDE_INDEX(va)]);
            pageDir.entry[PDE_INDEX(va)] = VM_NPRES;
        }
    }
}



// ======================== 测试 =====================


#ifdef TEST
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

#endif //TEST