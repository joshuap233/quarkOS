//
// Created by pjs on 2021/2/1.
//
// 页表使用递归映射
#ifndef QUARKOS_MM_KVM_H
#define QUARKOS_MM_KVM_H

#include <types.h>
#include <mm/mm.h>
#include <mm/page.h>

#define CR3_CTRL 0          //不使用 write-through,且页目录允许缓存

typedef uint32_t cr3_t;
typedef uint32_t pde_t;
typedef uint32_t pte_t;


void kvm_unmap(struct page *page);

void kvm_unmap2(ptr_t addr);

void kvm_map(struct page *page, uint32_t flags);

ptr_t kvm_vm2pm(ptr_t va);

ptr_t kvm_pm2vm(ptr_t pa);

void kvm_copy(pde_t *pgdir);

struct page *kvm_vm2page(ptr_t va);

struct page *va_get_page(ptr_t addr);

void switch_kvm();

void switch_uvm(pde_t *pgdir);

void kvm_maps(ptr_t va, ptr_t pa, size_t size, u32_t flags);


#endif //QUARKOS_MM_KVM_H
