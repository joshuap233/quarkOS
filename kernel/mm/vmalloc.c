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
//int32_t vm_area_insert(mm_struct_t *mm, struct vm_area *area) {
//    list_head_t *hdr;
//    struct vm_area *entry;
//    list_for_each(hdr, &mm->area) {
//        entry = area_entry(hdr);
//        if (area->addr < entry->addr) break;
//    }
//    if (hdr != &mm->area) {
//        entry = area_entry(hdr);
//        if (entry->addr < area->addr + area->size) {
//            assertk(0);
//            return -1;
//        }
//    }
//    list_add_prev(&area->head, hdr);
//    return 0;
//}

// 用于初始化第一个用户任务
// 传入的 txt ,rodata ,dataBss 为物理地址,
// 且直接映射到虚拟内存
struct mm_struct *mm_struct_init(
        ptr_t text, ptr_t size1,
        ptr_t rodata, ptr_t size2,
        ptr_t dataBss, ptr_t size3) {
    assertk(text + size1 < 3 * G);
    assertk(rodata + size2 < 3 * G);
    assertk(dataBss + size3 < 3 * G - PAGE_SIZE);
    assertk(size1 > 0);

    struct mm_struct *mm = kmalloc(sizeof(struct mm_struct));
    vm_map_init(mm);

    // 初始化五个区域
    size1 = PAGE_ALIGN(size1);
    size2 = PAGE_ALIGN(size2);
    size3 = PAGE_ALIGN(size3);

    mm->size = size1 + size2 + size3;

    mm->text.addr = text;
    mm->text.size = size1;
    mm->text.flag = VM_PRES | VM_UR;
    vm_map(&mm->text, mm->text.addr, mm->pgdir);

    mm->rodata.size = size2;
    if (size2 > 0) {
        mm->rodata.addr = rodata;
        mm->rodata.flag = VM_PRES | VM_UR;
        vm_map(&mm->rodata, mm->rodata.addr, mm->pgdir);
    }

    mm->dataBss.size = size3;
    if (size3 > 0) {
        mm->dataBss.addr = dataBss;
        mm->dataBss.flag = VM_PRES | VM_UW;
        vm_map(&mm->dataBss, mm->dataBss.addr, mm->pgdir);
    }

    mm->brk.addr = dataBss + PAGE_SIZE;
    mm->brk.size = 0;
    mm->brk.flag = VM_PRES | VM_UW;


    // stack 可以改为随机地址
    mm->stack.addr = HIGH_MEM - 2 * PAGE_SIZE;
    mm->stack.size = PAGE_SIZE;
    mm->stack.flag = VM_PRES | VM_UW;

    // 设置页面的虚拟地址(对应用户页表)
    struct page *page = __alloc_page(PAGE_SIZE);
    page->data = (void *) mm->stack.addr;
    vm_map(&mm->stack, page_addr(page), mm->pgdir);

    return mm;
}

static void vm_map_init(struct mm_struct *mm) {
    mm->pgdir = kcalloc(PAGE_SIZE);
}

// 返回页表项地址
INLINE pte_t *vm_page_iter(ptr_t addr, pde_t *pgdir, bool new) {
    // *pde 为页表物理地址
    pde_t *pde = &pgdir[PDE_INDEX(addr)];
    pte_t *pte;
    if (new && !(*pde & VM_PRES)) {
        // 在内核空间映射页表
        pte = kcalloc(PAGE_SIZE);
        *pde = kvm_vm2pm((ptr_t) pte) | VM_PRES | VM_UW;
    } else {
        struct page *page = get_page(PAGE_ADDR(*pde));
        pte = page->data;
    }
    return &pte[PTE_INDEX(addr)];
}

int vm_remap_page(ptr_t va, pte_t *pgdir) {
    // 重新映射 va 所在的页,并复制原页内容
    struct page *new, *old;
    pte_t *pte = vm_page_iter(va, pgdir, true);

    if (!(*pte & VM_PRES)) return -1;

    old = get_page(PAGE_ADDR(*pte));

    // 分配新页,临时映射到内核用于复制页内容
    new = __alloc_page(PAGE_SIZE);
    kvm_map(new, VM_PRES | VM_KW);
    q_memcpy(new->data, (void *) va, PAGE_SIZE);

    // 取消临映射
    kvm_unmap(new);

    //映射到用户空间
    *pte = (*pte & PAGE_MASK) | page_addr(new);

    __free_page(old);
    return 0;
}

void vm_map(struct vm_area *area, ptr_t pa, pte_t *pgdir) {
    ptr_t size = area->size;
    ptr_t va = area->addr;
    while (size > 0) {
        pte_t *pte = vm_page_iter(va, pgdir, true);
        assertk(!(*pte & VM_PRES));
        *pte = pa | area->flag;
        pa += PAGE_SIZE;
        va += PAGE_SIZE;
        size -= PAGE_SIZE;
    }
}


void vm_unmap(struct vm_area *area, pte_t *pgdir) {
    ptr_t addr = area->addr;
    int64_t size = area->size;
    while (size > 0) {
        pte_t *pte = vm_page_iter(addr, pgdir, false);
        assertk(*pte & VM_PRES);
        free_page(PAGE_ADDR(*pte));
        *pte = VM_NPRES;
        addr += PAGE_SIZE;
        size -= PAGE_SIZE;
    }
}

ptr_t vm_vm2pm(void *addr, pte_t *pgdir) {
    pte_t *pte = vm_page_iter((ptr_t) addr, pgdir, false);
    return PAGE_ADDR(*pte);
}

void vm_struct_destroy(struct mm_struct *mm) {
    vm_unmap(&mm->text, mm->pgdir);
    vm_unmap(&mm->dataBss, mm->pgdir);
    vm_unmap(&mm->rodata, mm->pgdir);
    vm_unmap(&mm->brk, mm->pgdir);
    vm_unmap(&mm->stack, mm->pgdir);

    // 回收页表
    for (u32_t i = 0; i < N_PDE / 4 * 3; ++i) {
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
    while (size > 0) {
        pte_t *pte = vm_page_iter(addr, sPgdir, false);
        pte_t *new = vm_page_iter(addr, dPgdir, true);
        if ((*pte & VM_UW) == VM_UW) {
            // 标记当前页的原本读写属性
            *pte &= VM_IGNORE_BIT1;
            CLEAR_BIT(*pte, VM_RW_BIT);
        }
        *new = *pte;

        page = get_page2(PAGE_ADDR(*pte));
        if (page != NULL)
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
    // TODO: 不复制整个 4K 栈
    pde = vm_page_iter(kStart, sPgdir, false);
    new = vm_page_iter(kStart, dPgdir, true);
    assertk(*pde && !(*new));
    stack = kmalloc(PAGE_SIZE);
    *new = kvm_vm2pm((ptr_t) stack) | VM_PRES | VM_UW;
    q_memcpy(stack, (void *) kvm_pm2vm(PAGE_ADDR(*pde)), PAGE_SIZE);


    size -= PAGE_SIZE;
    vm_area_copy(addr, size, sPgdir, dPgdir);
}


struct mm_struct *vm_struct_copy(struct mm_struct *src) {
    // 需要调用重新加载 cr3 以刷新 tlb 缓存
    // 复制虚拟页表并将页表标记为只读

    struct mm_struct *new = kmalloc(sizeof(struct mm_struct));
    q_memcpy(new, src, sizeof(struct mm_struct));
    vm_map_init(new);

    vm_area_copy(src->text.addr, src->text.size, src->pgdir, new->pgdir);
    vm_area_copy(src->rodata.addr, src->rodata.size, src->pgdir, new->pgdir);
    vm_area_copy(src->dataBss.addr, src->dataBss.size, src->pgdir, new->pgdir);
    vm_area_copy(src->brk.addr, src->brk.size, src->pgdir, new->pgdir);
    vm_area_copy_stack(&src->stack, src->pgdir, new->pgdir);
    return new;
}

