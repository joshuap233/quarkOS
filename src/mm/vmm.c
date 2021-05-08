//
// Created by pjs on 2021/2/1.
//
//虚拟内存管理
#include "mm/vmm.h"
#include "mm/page_alloc.h"
#include "lib/qstring.h"
#include "lib/qlib.h"
#include "mm/vm_area.h"

//内核页表本身在页表内的索引
#define K_PD_INDEX        1023
//PDE 虚拟地址
#define K_PTE_VA         ((ptr_t)K_PD_INDEX << 22)
//返回页目录索引为index的页表首地址
#define PTE_ADDR(pde_index) (K_PTE_VA + ((pde_index) << 12))

// 返回 va 对应的页目录项虚拟地址
#define PDE(va) (pageDir.entry[PDE_INDEX(va)])
// 返回 va 对应的页表首地址
#define PTB(va) PTE_ADDR(PDE_INDEX(va))
// 返回页目录项地址
#define PTE(pt, va) ((pt)->entry[PTE_INDEX(va)])

extern void cr3_set(ptr_t);

static ptb_t *getPageTable(ptr_t va, u32_t flags);

static pdr_t _Alignas(PAGE_SIZE) pageDir = {
        .entry = {[0 ...PAGE_ENTRY_NUM - 1]=(VM_KR | VM_NPRES)}
};


// 初始化内核页表
void vmm_init() {
    cr3_t cr3 = CR3_CTRL | ((ptr_t) &pageDir);
    pageDir.entry[N_PDE - 1] = (ptr_t) &pageDir | VM_KW | VM_PRES;

    struct vm_area *vm_area;
    for_each_vm_area(vm_area) {
        if (vm_area->type == KERNEL_AREA) {
            struct area *area;
            for_each_area(area, vm_area->area) {
                assertk((area->size & PAGE_MASK) == 0);
                vmm_mapd(area->addr, area->addr, area->size, area->flag);
            }
            break;
        }
    }

    cr3_set(cr3);
}

// 映射页目录
static ptb_t *getPageTable(ptr_t va, u32_t flags) {
    bool paging = is_paging();
    pde_t *pde = &PDE(va);
    ptb_t *pt = (ptb_t *) (paging ? PTB(va) : PAGE_ADDR(*pde));

    if (!(*pde & VM_PRES)) {
        ptr_t pa;
        assertk((pa = pm_alloc_page()) != PMM_NULL);
        *pde = pa | flags;
        if (!paging) pt = (ptb_t *) pa;
        q_memset(pt, 0, sizeof(ptb_t));
    }
    return pt;
}

void vmm_mapPage(ptr_t va, ptr_t pa, u32_t flags) {
    assertk((va & PAGE_MASK) == 0);
    ptb_t *pt = getPageTable(va, flags);
    PTE(pt, va) = pa | flags;
    tlb_flush(va);
}

// 直接映射
void vmm_mapd(ptr_t va, ptr_t pa, u32_t size, u32_t flags) {
    assertk((va & PAGE_MASK) == 0);
    ptr_t end = va + size;
    for (; va < end; pa += PAGE_SIZE, va += PAGE_SIZE) {
        vmm_mapPage(va, pa, flags);
    }
}

// 映射 va ~ va+size-1
void vmm_mapv(ptr_t va, u32_t size, u32_t flags) {
    assertk((va & PAGE_MASK) == 0);
    ptr_t end = va + size;
    for (; va < end; va += PAGE_SIZE) {
        ptr_t pa;
        assertk((pa = pm_alloc_page()) != PMM_NULL);
        vmm_mapPage(va, pa, flags);
    }
}


// 使用虚拟地址找到物理地址
ptr_t vmm_vm2pm(ptr_t va) {
    ptb_t *pt = (ptb_t *) PTB(va);
    return PAGE_ADDR(PTE(pt, va)) + (va & PAGE_MASK);
}

void vmm_unmapPage(ptr_t va) {
    ptb_t *pt = (ptb_t *) PTB(va);
    PTE(pt, va) = VM_NPRES;
    tlb_flush(va);
}

// size 为需要释放的内存大小
void vmm_unmap(void *va, u32_t size) {
    assertk((size & PAGE_MASK) == 0);
    void *end = va + size;
    for (; va < end; va += PAGE_SIZE) {
        vmm_unmapPage((ptr_t) va);
    }
}


void vmm_recycle() {
    // TODO: 内存不足时再回收 unmap 没有释放的空页表
}
