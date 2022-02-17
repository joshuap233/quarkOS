//
// Created by pjs on 2021/5/30.
//

#ifndef QUARKOS_MM_VMALLOC_H
#define QUARKOS_MM_VMALLOC_H

#include <types.h>
#include <lib/list.h>
#include <mm/vm.h>

struct mm_args {
    ptr_t text; // 虚拟地址
    ptr_t pa1;  // 物理地址
    ptr_t size1;// 节大小

    ptr_t rodata;
    ptr_t pa2;
    ptr_t size2;

    ptr_t data;
    ptr_t pa3;
    ptr_t size3;

    ptr_t bss;
    ptr_t pa4;
    ptr_t size4;
};

typedef struct mm_struct {
    struct vm_area {
        ptr_t va;       // 虚拟地址
        u32_t size;
        u32_t flag;
    } text, rodata, data, bss, brk, stack;
    ptr_t size;           // 已经使用的虚拟内存大小
    pde_t *pgdir;         // 页目录物理地址
} mm_struct_t;


void vm_struct_destroy(struct mm_struct *mm);

void vm_stack_init(struct mm_struct *mm);

void vm_brk_init(struct mm_struct *mm);

ptr_t vm_vm2pm(void *addr, pte_t *pgdir);

void vm_unmap(struct vm_area *area, pte_t *pgdir);

struct mm_struct *vm_struct_copy(struct mm_struct *src);

int vm_remap_page(ptr_t va, pte_t *pgdir);

void vm_struct_unmaps(struct mm_struct *mm);

void vm_maps(ptr_t va, ptr_t pa, u32_t flag, ptr_t size);

void vm_map_init(struct mm_struct *mm);

void mm_struct_init(struct mm_struct *mm, struct mm_args *args);

#endif //QUARKOS_MM_VMALLOC_H
