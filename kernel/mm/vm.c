//
// Created by pjs on 2021/2/1.
// kernel virtual memory 管理

#include <mm/vm.h>
#include <mm/page_alloc.h>
#include <lib/qstring.h>
#include <lib/qlib.h>
#include <mm/block_alloc.h>

static pde_t _Alignas(PAGE_SIZE) pageDir[N_PTE] = {
        [0 ...N_PTE - 1]=VM_NPRES
};

// 内核部分页表
_Alignas(PAGE_SIZE) static pte_t kPageTables[N_PTE / 4][N_PTE] = {
        [0 ...N_PTE / 4 - 1] = {[0 ...N_PTE - 1]=VM_NPRES}
};
cr3_t kCr3;

static void kvm_page_init(ptr_t va, size_t size, u32_t flags);
void kvm_maps4(ptr_t va, ptr_t pa, size_t size, u32_t flags);

void load_cr3(){
    lcr3(kCr3);
}
// 初始化内核页表, 需要在在物理内存管理器初始化前调用,
// 因为物理内存管理器初始化消耗的内存(初始化page结构)可能超过临时页表映射的 4M
void kvm_init() {
    // block 分配器预留物理内存
    pmm_init_mm();
    g_mem_start = PAGE_ALIGN(block_start());

    ptr_t startKernel = (ptr_t) _startKernel;
    ptr_t dataStart = (ptr_t) _dataStart;
    ptr_t endKernel = (ptr_t) _endKernel;

    assertk(g_mem_start > endKernel - KERNEL_START);

    pde_t *pdr = &pageDir[PDE_INDEX(KERNEL_START)];
    for (u32_t i = 0; i < N_PTE / 4; ++i) {
        pdr[i] = ((ptr_t) &kPageTables[i] - KERNEL_START) | VM_KRW | VM_PRES;
    }

    kCr3 = CR3_CTRL | (((ptr_t) &pageDir) - KERNEL_START);
    // 映射 1M 以下区域
    kvm_page_init(KERNEL_START, startKernel - KERNEL_START, VM_KRW);

    // text 段 与 rodada 段
    kvm_page_init(startKernel, dataStart - startKernel, VM_KR);

    // data 段, bss 段 与初始化内核分配的内存
    kvm_page_init(dataStart, g_mem_start - (dataStart - KERNEL_START), VM_KRW);

    kvm_maps4(DEV_SPACE,DEV_SPACE, DEVSP_SIZE, VM_PRES|VM_KRW);
    load_cr3();
}



INLINE pte_t *getPageTableEntry(ptr_t va) {
    assertk(va >= KERNEL_START);
    return &((u32_t *) kPageTables)[(va - KERNEL_START) >> 12];
}

static void kvm_page_init(ptr_t va, size_t size, u32_t flags) {
    assertk((va & PAGE_MASK) == 0);

    ptr_t pa = va - KERNEL_START;
    ptr_t end = pa + size;
    pte_t *pte = getPageTableEntry(va);
    for (; pa < end; pa += PAGE_SIZE) {
        assertk((*pte & VM_PRES) == 0);
        *pte = pa | VM_PRES | flags;
        pte++;
    }
}

void kvm_maps4(ptr_t va, ptr_t pa, size_t size, u32_t flags) {
    assertk((va & PAGE_MASK) == 0);
    u32_t vpa  = pa>>12;
    ptr_t end = vpa + DIV_CEIL(size,PAGE_SIZE);
    pte_t *pte = getPageTableEntry(va);

    for (; vpa < end; vpa ++) {
        assertk((*pte & VM_PRES) == 0);
        *pte = (vpa<<12) | VM_PRES | flags;
        pte++;
    }
}

void kvm_maps(ptr_t va, ptr_t pa, size_t size, u32_t flags) {
    assertk((va & PAGE_MASK) == 0);
    ptr_t end = va + size;
    pte_t *pte = getPageTableEntry(va);
    for (; va < end; pa += PAGE_SIZE, va += PAGE_SIZE) {
        assertk((*pte & VM_PRES) == 0);
        *pte = pa | flags;
        pte++;
        tlb_flush(va);
    }
}

void kvm_map(struct page *page, u32_t flags) {
    ptr_t pa = page_addr(page);
    ptr_t va = pa +KERNEL_START;
    assertk(pa<PHYS_TOP);

    page->data = (void *) va;
    ptr_t size = page->size;
    kvm_maps(va, pa, size, flags);
}

struct page *va_get_page(ptr_t addr) {
    addr = v2p(addr);
    assertk(addr != 0);
    struct page *page = get_page(addr);
    if (!page || !page_head(page))
        return NULL;
    return page;
}

struct page *kvm_vm2page(ptr_t va) {
    pte_t *pte = getPageTableEntry(va);
    return get_page(PAGE_ADDR(*pte));
}

ptr_t v2p(ptr_t va) {
    pte_t *pte = getPageTableEntry(va);
    return PAGE_ADDR(*pte);
}

ptr_t p2v(ptr_t pa) {
    struct page *page = get_page(pa);
    assertk(page);
    return (ptr_t) page->data;
}

// 释放临时映射
void kvm_unmap3(void *va, u32_t size){
    assertk((ptr_t) va > KERNEL_START);
    assertk((size & PAGE_MASK) == 0);

    void *end = va + size;
    pte_t *pte = getPageTableEntry((ptr_t) va);
    for (; va < end; va += PAGE_SIZE) {
        *pte = VM_NPRES;
        tlb_flush((ptr_t) va);
        pte++;
    }
}

// size 为需要释放的内存大小
void kvm_unmap(struct page *page) {
    void *va = page->data;
    u32_t size = page->size;
    kvm_unmap3(va,size);
}


void kvm_unmap2(ptr_t addr) {
    kvm_unmap(kvm_vm2page((ptr_t) addr));
}


void kvm_copy(pde_t *pgdir) {
    // 复制内核页表
    u32_t step = N_PDE / 4 * 3;
    pgdir += step;
    pde_t *kPageDir = pageDir + step;
    for (u32_t i = 0; i < N_PDE / 4; ++i) {
        if (kPageDir[i] & VM_PRES) {
            pgdir[i] = kPageDir[i];
        }
    }
}

void switch_kvm() {
    lcr3(kCr3);
}

void switch_uvm(pde_t *pgdir) {
    lcr3(v2p((ptr_t) pgdir));
}

void kvm_recycle() {
    // 回收 kvm_unmapPage 没有释放的物理页
}