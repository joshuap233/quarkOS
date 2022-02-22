//
// Created by pjs on 2021/5/30.
//
// 管理用户空间虚拟内存
#include <mm/uvm.h>
#include <mm/vm.h>
#include <mm/kmalloc.h>
#include <mm/page_alloc.h>
#include <lib/qlib.h>
#include <lib/list.h>
#include <lib/qstring.h>
#include <task/task.h>

/**
 *  kernel
 *  --------
 *   4K padding
 *  -------- 3G
 *  PAGE_SIZE
 *  --------
 *  stack    ↓
 *
 *
 *  brk      ↑
 *  --------
 *  PAGE_SIZE
 *  --------
 *  data/bss
 *  --------
 *  rodata
 *  --------
 *  text
 *  --------
 *  PAGE_SIZE
 *  -------- 0
 */

static void vm_map_area(struct vm_area *area, ptr_t pa, pte_t *pgdir);


// 用于初始化第一个用户任务
// 传入的 txt ,rodata ,data,bss 为物理地址,
void mm_struct_init(struct mm_struct *mm, struct mm_args *args) {
    assertk(mm);
    assertk(args->text + args->size1 < 3 * G);
    assertk(args->rodata + args->size2 < 3 * G);
    assertk(args->data + args->size3 < 3 * G);
    assertk(args->size1 > 0);

    // 初始化六个区域
    size_t size1 = PAGE_FLOOR(args->size1);
    size_t size2 = PAGE_FLOOR(args->size2);
    size_t size3 = PAGE_FLOOR(args->size3);
    size_t size4 = PAGE_FLOOR(args->size4);

    mm->size = size1 + size2 + size3 + size4;

    mm->text.va = args->text;
    mm->text.size = size1;
    mm->text.flag = VM_PRES | VM_UR;
    vm_map_area(&mm->text, args->pa1, mm->pgdir);

    mm->rodata.size = size2;
    if (size2 > 0) {
        mm->rodata.va = args->rodata;
        mm->rodata.flag = VM_PRES | VM_UR;
        vm_map_area(&mm->rodata, args->pa2, mm->pgdir);
    }

    mm->data.size = size3;
    if (size3 > 0) {
        mm->data.va = args->data;
        mm->data.flag = VM_PRES | VM_URW;
        vm_map_area(&mm->data, args->pa3, mm->pgdir);
    }

    mm->bss.size = size3;
    if (size4 > 0) {
        mm->bss.va = args->data;
        mm->bss.flag = VM_PRES | VM_URW;
        vm_map_area(&mm->bss, args->pa4, mm->pgdir);
    }

    vm_brk_init(mm);
    vm_stack_init(mm);
}

void vm_brk_init(struct mm_struct *mm) {
    u32_t brk = MAX(mm->bss.va,mm->text.va);
    brk = MAX(brk,mm->data.va);
    brk = MAX(brk,mm->rodata.va);
    brk = PAGE_CEIL(brk) + PAGE_SIZE;
    assertk(brk < KERNEL_START - 2 * PAGE_SIZE);
    mm->brk.va = brk;
    mm->brk.size = 0;
    mm->brk.flag = VM_PRES | VM_URW;
}

void vm_stack_init(struct mm_struct *mm) {
    // 设置用户栈
    mm->stack.va = KERNEL_START - 2 * PAGE_SIZE;
    mm->stack.size = PAGE_SIZE;
    mm->stack.flag = VM_PRES | VM_URW;
    mm->size += PAGE_SIZE;

    struct page *page = __alloc_page(PAGE_SIZE);
    page->data = (void *) mm->stack.va;
    vm_map_area(&mm->stack, page_addr(page), mm->pgdir);
    tlb_flush(mm->stack.va);
}

void vm_map_init(struct mm_struct *mm) {
    mm->pgdir = kcalloc(PAGE_SIZE);
}

// 返回页表项地址
static pte_t *vm_page_iter(ptr_t va, pde_t *pgdir, bool new) {
    // *pde 为页表物理地址
    pde_t *pde = &pgdir[PDE_INDEX(va)];
    pte_t *pte;
    if (new && !(*pde & VM_PRES)) {
        // 在内核空间映射用户页表
        pte = kcalloc(PAGE_SIZE);
        *pde = v2p((ptr_t) pte) | VM_PRES | VM_URW;
    } else {
        struct page *page = get_page(PAGE_CEIL(*pde));
        assertk(page);
        pte = page->data;
    }
    return &pte[PTE_INDEX(va)];
}

int vm_remap_page(ptr_t va, pte_t *pgdir) {
    // 重新映射 va 所在的页,并复制原页内容
    struct page *new, *old;
    pte_t *pte = vm_page_iter(va, pgdir, true);

    if (!(*pte & VM_PRES)) return -1;

    old = get_page(PAGE_CEIL(*pte));
    assertk(old);

    // 分配新页,临时映射到内核用于复制页内容
    new = __alloc_page(PAGE_SIZE);
    kvm_map(new, VM_PRES | VM_KRW);
    memcpy(new->data, (void *) va, PAGE_SIZE);

    // 取消临映射
    kvm_unmap(new);

    //映射到用户空间
    *pte = (*pte & PAGE_MASK) | page_addr(new);

    __free_page(old);
    return 0;
}

static void vm_map_area(struct vm_area *area, ptr_t pa, pte_t *pgdir) {
    int64_t size = area->size;
    ptr_t va = area->va;
    while (size > 0) {
        pte_t *pte = vm_page_iter(va, pgdir, true);
        assertk(!(*pte & VM_PRES));
        *pte = pa | area->flag;
        pa += PAGE_SIZE;
        va += PAGE_SIZE;
        size -= PAGE_SIZE;
    }
}

void vm_maps(ptr_t va, ptr_t pa, u32_t flag, ptr_t _size) {
    // 映射当前任务页表
    assertk(CUR_TCB->mm);

    pde_t *pgdir = CUR_TCB->mm->pgdir;
    assertk((pa & PAGE_MASK) == 0);
    int64_t size = _size;
    va = PAGE_CEIL(va);
    while (size > 0) {
        pte_t *pte = vm_page_iter(va, pgdir, true);
        if (!(*pte & VM_PRES)) {
            *pte = pa | flag;
        }
        pa += PAGE_SIZE;
        va += PAGE_SIZE;
        size -= PAGE_SIZE;
        tlb_flush(va);
    }
}


void vm_unmap(struct vm_area *area, pte_t *pgdir) {
    ptr_t addr = area->va;
    int64_t size = area->size;
    int errno;
    while (size > 0) {
        pte_t *pte = vm_page_iter(addr, pgdir, false);
        assertk(*pte & VM_PRES);
        errno = free_page(PAGE_CEIL(*pte));
        if (errno) return;

        *pte = VM_NPRES;
        addr += PAGE_SIZE;
        size -= PAGE_SIZE;
    }
}

ptr_t vm_vm2pm(void *addr, pte_t *pgdir) {
    pte_t *pte = vm_page_iter((ptr_t) addr, pgdir, false);
    return PAGE_CEIL(*pte);
}

void vm_struct_unmaps(struct mm_struct *mm) {
    // 取消用户空间映射
    vm_unmap(&mm->text, mm->pgdir);
    vm_unmap(&mm->data, mm->pgdir);
    vm_unmap(&mm->bss, mm->pgdir);
    vm_unmap(&mm->rodata, mm->pgdir);
    vm_unmap(&mm->brk, mm->pgdir);
    vm_unmap(&mm->stack, mm->pgdir);

    // 回收页表
    for (u32_t i = 0; i < N_PDE / 4 * 3; ++i) {
        pte_t pde = mm->pgdir[i];
        if (pde & VM_PRES) {
            struct page *page = get_page(PAGE_CEIL(pde));
            assertk(page);
            kvm_unmap(page);
            __free_page(page);
        }
    }
}

void vm_struct_destroy(struct mm_struct *mm) {
    vm_struct_unmaps(mm);

    // 回收页目录
    kfree(mm->pgdir);
    kfree(mm);
}

static void vm_area_copy(
        ptr_t addr, ptr_t _size,
        pde_t *sPgdir, pde_t *dPgdir) {
    int64_t size = _size;
    struct page *page;
    while (size > 0) {
        pte_t *pte = vm_page_iter(addr, sPgdir, false);
        pte_t *new = vm_page_iter(addr, dPgdir, true);
        if ((*pte & VM_URW) == VM_URW) {
            // 标记当前页的原本读写属性
            *pte &= VM_IGNORE_BIT1;
            CLEAR_BIT(*pte, VM_RW_BIT);
        }
        *new = *pte;

        page = get_page(PAGE_CEIL(*pte));
        if (page) page->ref_cnt++;

        addr += PAGE_SIZE;
        size -= PAGE_SIZE;
    }
}

// 复制用户空间栈
static void vm_area_copy_stack(
        struct vm_area *src, pde_t *sPgdir, pde_t *dPgdir) {
    pde_t *pde, *new;
    ptr_t addr = src->va;
    int64_t size = src->size;
    ptr_t kStart = addr + size - PAGE_SIZE;
    void *stack;

    // 复制栈顶部 4K 数据,其余区域设置为只读
    pde = vm_page_iter(kStart, sPgdir, false);
    new = vm_page_iter(kStart, dPgdir, true);
    assertk(*pde && !(*new));

    // 临时映射到内核用于复制
    stack = kmalloc(PAGE_SIZE);
    *new = v2p((ptr_t) stack) | VM_PRES | VM_URW;
    memcpy(stack, (void *) p2v(PAGE_CEIL(*pde)), PAGE_SIZE);

    // 取消临时映射
    kvm_unmap2((ptr_t) stack);

    size -= PAGE_SIZE;
    vm_area_copy(addr, size, sPgdir, dPgdir);
}

struct mm_struct *vm_struct_copy(struct mm_struct *src) {
    struct mm_struct *new = kmalloc(sizeof(struct mm_struct));
    memcpy(new, src, sizeof(struct mm_struct));
    vm_map_init(new);

    vm_area_copy(src->text.va, src->text.size, src->pgdir, new->pgdir);
    vm_area_copy(src->rodata.va, src->rodata.size, src->pgdir, new->pgdir);
    vm_area_copy(src->data.va, src->data.size, src->pgdir, new->pgdir);
    vm_area_copy(src->bss.va, src->bss.size, src->pgdir, new->pgdir);
    vm_area_copy(src->brk.va, src->brk.size, src->pgdir, new->pgdir);
    vm_area_copy_stack(&src->stack, src->pgdir, new->pgdir);

    // 复制内核页表
    kvm_copy(new->pgdir);

    // 复制虚拟页表并将页表标记为只读
    // 调用重新加载 cr3 以刷新 tlb 缓存
    lcr3(v2p((ptr_t) CUR_TCB->mm->pgdir));
    return new;
}

u32_t sbrk(u32_t size){
    struct mm_struct *cur = CUR_TCB->mm;
    assertk(cur);
    size = PAGE_CEIL(size);
    struct page *page = __alloc_page(size);
    vm_maps(cur->brk.va,page_addr(page),
            VM_PRES|VM_URW,size);
    page->data = (void*)cur->brk.va;
    cur->brk.va += size;
    cur->brk.size += size;
    return cur->brk.va - size;
}