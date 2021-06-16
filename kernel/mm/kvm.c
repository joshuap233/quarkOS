//
// Created by pjs on 2021/2/1.
//
// 内核空间虚拟内存管理

#include <mm/kvm.h>
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
static cr3_t kCr3;

static void kvm_page_init(ptr_t va, size_t size, u32_t flags);

// 初始化内核页表, 需要在在物理内存管理器初始化前调用,
// 因为物理内存管理器初始化消耗的内存(初始化page结构)可能超过临时页表映射的 4M
void kvm_init() {
    // block 分配器预留物理内存
    pmm_init_mm();
    g_mem_start = PAGE_ALIGN(block_start());

    ptr_t startKernel = (ptr_t) _startKernel;
    ptr_t dataStart = (ptr_t) _dataStart;
    ptr_t endKernel = (ptr_t) _endKernel;
    assertk(g_mem_start > endKernel - HIGH_MEM);

    pde_t *pdr = &pageDir[PDE_INDEX(HIGH_MEM)];
    for (u32_t i = 0; i < N_PTE / 4; ++i) {
        pdr[i] = ((ptr_t) &kPageTables[i] - HIGH_MEM) | VM_KW | VM_PRES;
    }

    kCr3 = CR3_CTRL | (((ptr_t) &pageDir) - HIGH_MEM);

    // 低于 1M 的内存区域
    kvm_page_init(HIGH_MEM, startKernel - HIGH_MEM, VM_KW);

    // text 段 与 rodada 段
    kvm_page_init(startKernel, dataStart - startKernel, VM_KR);

    // data 段, bss 段 与初始化内核分配的内存
    kvm_page_init(dataStart, g_mem_start - (dataStart - HIGH_MEM), VM_KW);

    lcr3(kCr3);
}

INLINE pte_t *getPageTableEntry(ptr_t va) {
    assertk(va >= HIGH_MEM);
    return &((u32_t *) kPageTables)[(va - HIGH_MEM) >> 12];
}

static void kvm_page_init(ptr_t va, size_t size, u32_t flags) {
    assertk((va & PAGE_MASK) == 0);

    ptr_t pa = va - HIGH_MEM;
    ptr_t end = pa + size;
    pte_t *pte = getPageTableEntry(va);
    for (; pa < end; pa += PAGE_SIZE) {
        *pte = pa | VM_PRES | flags;
        pte++;
    }
}


// 返回实际映射的虚拟地址
void kvm_map(struct page *page, u32_t flags) {
    ptr_t pa = page_addr(page);
    ptr_t va = pa;

    // 3G 以上的物理内存不足
    if (va < HIGH_MEM) {
        va = HIGH_MEM + (pa & (1 * G - 1));
    }
    page->data = (void *) va;

    //TODO: 如果已经被映射则查找可用地址空间
    ptr_t size = page->size;
    ptr_t end = va + size;
    pte_t *pte = getPageTableEntry(va);
    for (; va < end; pa += PAGE_SIZE, va += PAGE_SIZE) {
        assertk((*pte & VM_PRES) == 0);
        *pte = pa | flags;
        pte++;
        tlb_flush(va);
    }
}

struct page *va_get_page(ptr_t addr) {
    addr = kvm_vm2pm(addr);
    assertk(addr != 0);
    struct page *page = get_page(addr);
    if (!page || !page_head(page))
        return NULL;
    return page;
}

// 使用虚拟地址找到物理地址
ptr_t kvm_vm2pm(ptr_t va) {
    pte_t *pte = getPageTableEntry(va);
    return PAGE_ADDR(*pte);
}

ptr_t kvm_pm2vm(ptr_t pa) {
    struct page *page = get_page(pa);
    assertk(page);
    return (ptr_t) page->data;
}

struct page *kvm_vm2page(ptr_t va) {
    pte_t *pte = getPageTableEntry(va);
    return get_page(PAGE_ADDR(*pte));
}

// size 为需要释放的内存大小
void kvm_unmap(struct page *page) {
    void *va = page->data;
    u32_t size = page->size;

    assertk((ptr_t) va > HIGH_MEM);
    assertk((size & PAGE_MASK) == 0);

    void *end = va + size;
    pte_t *pte = getPageTableEntry((ptr_t) va);
    for (; va < end; va += PAGE_SIZE) {
        *pte = VM_NPRES;
        tlb_flush((ptr_t) va);
        pte++;
    }
}

void kvm_unmap2(ptr_t addr){
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
    lcr3(kvm_vm2pm((ptr_t) pgdir));
}

void kvm_recycle() {
    // 回收 kvm_unmapPage 没有释放的物理页
}