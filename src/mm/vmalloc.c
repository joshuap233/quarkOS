//
// Created by pjs on 2021/5/30.
//
// 管理用户空间虚拟内存
// TODO: 管理 brk
#include <mm/vmalloc.h>
#include <mm/mm.h>
#include <mm/kvm.h>
#include <mm/kmalloc.h>
#include <mm/page_alloc.h>
#include <lib/qlib.h>
#include <lib/list.h>
#include <lib/qstring.h>


/**
 *  kernel
 *  -------- 3G
 *  --------
 *  stack    ↓
 *
 *
 *  heap     ↑
 *  --------
 *  data/bss
 *  --------
 *  rodata
 *  --------
 *  text
 *  -------- 0
 */

static void vm_map_init(struct mm_struct *mm);

#define area_entry(ptr) list_entry(ptr,struct vm_area,head)


// 按照 addr 大小递增插入
int32_t vm_area_insert(mm_struct_t *mm, struct vm_area *area) {
    list_head_t * hdr;
    struct vm_area *entry;
    list_for_each(hdr, &mm->area) {
        entry = area_entry(hdr);
        if (area->addr < entry->addr) break;
    }
    if (hdr != &mm->area) {
        entry = area_entry(hdr);
        if (entry->addr < area->addr + area->size) {
            assertk(0);
            return -1;
        }
    }
    list_add_prev(&area->head, hdr);
    return 0;
}


struct mm_struct *mm_struct_init(
        ptr_t txt, ptr_t size1,
        ptr_t rodata, ptr_t size2,
        ptr_t dataBss, ptr_t size3) {
    assertk(txt + size1 < 3 * G);
    assertk(rodata + size2 < 3 * G);
    assertk(dataBss + size3 < 3 * G - PAGE_SIZE);
    struct mm_struct *mm = kmalloc(sizeof(struct mm_struct));

    // 初始化五个区域
    size1 = PAGE_ALIGN(size1);
    size2 = PAGE_ALIGN(size2);
    size3 = PAGE_ALIGN(size3);

    mm->size = size1 + size2 + size3;
    list_header_init(&mm->area);

    mm->text.addr = txt;
    mm->text.size = size1;
    mm->text.flag = VM_PRES | VM_UR;
    assertk(vm_area_insert(mm, &mm->text) == 0);

    mm->rodata.addr = rodata;
    mm->rodata.size = size2;
    mm->rodata.flag = VM_PRES | VM_UR;
    assertk(vm_area_insert(mm, &mm->rodata) == 0);

    mm->dataBss.addr = dataBss;
    mm->dataBss.size = size3;
    mm->dataBss.flag = VM_PRES | VM_UW;
    assertk(vm_area_insert(mm, &mm->dataBss) == 0);

    mm->brk.addr = dataBss + PAGE_SIZE;
    mm->brk.size = 0;
    mm->brk.flag = VM_PRES | VM_UW;
    assertk(vm_area_insert(mm, &mm->brk) == 0);

    // stack 可以改为随机地址
    mm->stack.addr = HIGH_MEM - PAGE_SIZE;
    mm->stack.size = PAGE_SIZE;
    mm->stack.flag = VM_PRES | VM_UW;
    assertk(vm_area_insert(mm, &mm->stack) == 0);

    // 初始化页表
    vm_map_init(mm);

    // 映射固定段
    list_head_t * hdr;
    list_for_each(hdr, &mm->area) {
        vm_unmap(area_entry(hdr), mm->pgdir);
    }

    return mm;
}

static void vm_map_init(struct mm_struct *mm) {
    mm->pgdir = kcalloc(PAGE_SIZE);
}

INLINE pde_t *vm_page_iter(ptr_t addr, pde_t *pgdir, bool new, u32_t flag) {
    pde_t *ptable = &pgdir[PDE_INDEX(addr)];
    void *va;
    if (new && !(*ptable & VM_PRES)) {
        // 在内核空间映射页表
        va = kcalloc(PAGE_SIZE);
        *ptable = kvm_vm2pm((ptr_t) va) | flag;
    } else {
        struct page *page = get_page(PAGE_ADDR(*ptable));
        va = page->data;
    }
    return va;
}

void vm_map(struct vm_area *area, pte_t *pgdir, u32_t flag) {
    ptr_t addr = area->addr;
    int64_t size = area->size;
    while (size < 0) {
        pde_t *pde = vm_page_iter(addr, pgdir, true, flag);
        assertk(!(*pde & VM_PRES));
        *pde = alloc_one_page() | area->flag;
        addr += PAGE_SIZE;
        size -= PAGE_SIZE;
    }
}


void vm_unmap(struct vm_area *area, pte_t *pgdir) {
    ptr_t addr = area->addr;
    int64_t size = area->size;
    while (size < 0) {
        pde_t *pde = vm_page_iter(addr, pgdir, false, 0);
        assertk(*pde & VM_PRES);
        free_page(PAGE_ADDR(*pde));
        *pde = VM_NPRES;
        addr += PAGE_SIZE;
        size -= PAGE_SIZE;
    }
}

ptr_t vm_vm2pm(void *addr, pte_t *pgdir) {
    pde_t *pde = vm_page_iter((ptr_t) addr, pgdir, false, 0);
    return PAGE_ADDR(*pde);
}

void vm_struct_destroy(struct mm_struct *mm) {
    struct vm_area *area;
    list_head_t * hdr;
    list_for_each(hdr, &mm->area) {
        area = area_entry(hdr);
        vm_unmap(area, mm->pgdir);
    }

    // 回收页表
    for (u32_t i = 0; i < N_PDE; ++i) {
        pte_t pde = mm->pgdir[i];
        if (pde & VM_PRES) {
            kfree((void *) PAGE_ADDR(pde));
        }
    }

    // 回收页目录
    kfree(mm->pgdir);
    kfree(mm);
}

static void vm_area_copy(
        ptr_t addr, ptr_t _size,
        pde_t *sPgdir, pde_t *dPgdir) {
    int64_t size = _size;
    struct page *page;
    while (size < 0) {
        pde_t *pde = vm_page_iter(addr, sPgdir, false, 0);
        pde_t *new = vm_page_iter(addr, dPgdir, true, *pde & PAGE_MASK);
        CLEAR_BIT(*pde, VM_RW_BIT);
        *new = *pde;
        page = kvm_vm2page(PAGE_ADDR(*pde));
        page->ref_cnt++;

        addr += PAGE_SIZE;
        size -= PAGE_SIZE;
    }
}

static void vm_area_copy_stack(
        struct vm_area *src, pde_t *sPgdir, pde_t *dPgdir) {
    pde_t *pde, *new;
    ptr_t addr = src->addr;
    int64_t size = src->size;
    ptr_t kStart = addr + size - PAGE_SIZE;
    void *stack;

    // 复制栈顶部 4K 数据,其余区域设置为只读
    pde = vm_page_iter(kStart, sPgdir, false, 0);
    new = vm_page_iter(kStart, dPgdir, true, VM_PRES | VM_UW);
    assertk(*pde && !(*new));
    stack = kmalloc(PAGE_SIZE);
    *pde = (ptr_t) stack | VM_PRES | VM_UW;
    q_memcpy(stack, (void *) PAGE_ADDR(*pde), PAGE_SIZE);


    size -= PAGE_SIZE;
    vm_area_copy(addr, size, sPgdir, dPgdir);
}


struct mm_struct *vm_struct_copy(struct mm_struct *src) {
    // 需要调用重新加载 cr3 以刷新 tlb 缓存
    // 复制虚拟页表并将页表标记为只读

    struct mm_struct *new = kmalloc(sizeof(struct mm_struct));
    q_memcpy(new, src, sizeof(struct mm_struct));
    vm_map_init(new);

    assertk(vm_area_insert(new, &new->text) == 0);
    assertk(vm_area_insert(new, &new->rodata) == 0);
    assertk(vm_area_insert(new, &new->dataBss) == 0);
    assertk(vm_area_insert(new, &new->brk) == 0);
    assertk(vm_area_insert(new, &new->stack) == 0);

    vm_area_copy(src->text.addr, src->text.size, src->pgdir, new->pgdir);
    vm_area_copy(src->rodata.addr, src->rodata.size, src->pgdir, new->pgdir);
    vm_area_copy(src->dataBss.addr, src->dataBss.size, src->pgdir, new->pgdir);
    vm_area_copy(src->brk.addr, src->brk.size, src->pgdir, new->pgdir);
    vm_area_copy_stack(&src->stack, src->pgdir, new->pgdir);
    return new;
}

