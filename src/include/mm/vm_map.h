//
// Created by pjs on 2021/5/30.
//

#ifndef QUARKOS_MM_VM_MAP_H
#define QUARKOS_MM_VM_MAP_H

#include <types.h>
#include <lib/list.h>

typedef struct mm_struct {
    struct vm_area {
        list_head_t head;
        ptr_t addr;
        u32_t size;
        u32_t flag;
    } _area[5];// 分别为 txt,rodata,data/bss,brk,stack
    list_head_t area;
    struct vm_area *brk;
    struct vm_area *stack; //用户栈
    ptr_t size;
} mm_struct_t;


void vm_area_add(ptr_t addr, u32_t size, u16_t flag);

void vm_area_init();


#endif //QUARKOS_MM_VM_MAP_H
