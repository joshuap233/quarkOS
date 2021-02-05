//
// Created by pjs on 2021/2/1.
//
//虚拟内存管理
#include "virtual_mm.h"
#include "types.h"
#include "mm.h"
#include "physical_mm.h"

typedef struct free_list {
    pointer_t addr;     //空闲空间起始地址
    pointer_t end_addr;
    uint32_t size;     //缓存一份页面数
    struct free_list *next;
} list_t;

static pde_t _Alignas(PAGE_SIZE) k_pde[] = {[0 ...PAGE_SIZE - 1]=(MM_KR | MM_NPRES)};
//内核虚拟页目录

static list_t free_list;

bool list_split(pointer_t va, uint32_t size) {
    //查找空闲列表是否有满足大小的空闲空间并切割
    va = PAGE_ADDR(va);
    pointer_t end = va + size * PAGE_SIZE;
    list_t *temp = &free_list, *pre_temp = temp;
    while (temp != MM_NULL) {
        if (temp->addr <= va && temp->end_addr >= end) {
            temp->size -= size;
            temp->addr = end;
            if (temp->addr == end) pre_temp->next = temp->next;
            return true;
        }
        pre_temp = temp;
        temp = temp->next;
    }
    return false;
}

// 使用首次适应查找,返回空闲页虚拟地址, size 为页面数
void *list_splitr(uint32_t size) {
    list_t *temp = &free_list, *pre_temp = temp;
    while (temp != MM_NULL) {
        if (temp->size >= size) {
            void *temp_addr = (void *) (temp->addr);
            temp->addr = temp->addr + size * PAGE_SIZE;
            temp->size -= size;
            if (temp->addr == temp->end_addr) pre_temp->next = temp->next;
            return temp_addr;
        }
        pre_temp = temp;
        temp = temp->next;
    }
    return MM_NULL;
}


//释放空闲空间
bool list_append(pointer_t va, uint32_t size) {
    list_t *temp = &free_list, *pre_temp = temp;
    while (temp != MM_NULL) {
        if (temp->size >= size) {
        //TODO:需要堆
        }
        pre_temp = temp;
        temp = temp->next;
    }
    return MM_NULL;
}


//合并连续空闲空间
void list_merge(){

}

void free_list_init() {
    free_list.addr = 0;
    free_list.end_addr = PHYMM - 1;
    free_list.size = N_VPAGE;
    free_list.next = MM_NULL;
}


// 初始化内核页表
void vmm_init() {
    free_list_init();
    uint32_t size = SIZE_ALIGN(K_END) / PAGE_SIZE;
    cr3_t cr3 = CR3_CTRL | ((pointer_t) &k_pde);
    //映射内核空间与低于 1M 的空间
    assertk(list_split(0, size));
    mm_map(0, 0, SIZE_ALIGN(K_END) / PAGE_SIZE);
    cr3_set(cr3);
}


// 三个参数分别为:
// 需要映射的虚拟地址,物理地址, 虚拟地址页面数, (地址需要包括20位地址与12位标志)
// 映射成功返回 true
bool mm_map(pointer_t va, pointer_t pa, uint32_t size) {
    uint32_t pdeI = PDE_INDEX(va);
    uint32_t pteI = PTE_INDEX(va);

    for (; size > 0; pdeI++, pteI = 0) {
        pte_t *pte = &k_pde[pdeI];
        if ((*pte & MM_PRES) == 0) {
            *pte = phymm_alloc() | MM_KW | MM_PRES;
            //TODO: va 为 *pte, 但如果 pte (虚拟地址)已经被分配了呢
            if (!mm_map(*pte, *pte, 1)) return false;
        }
        for (; pteI < N_PTE; pteI++, size--, pa += PAGE_SIZE)
            pte[pteI] = pa;
    }
    return true;
}



//size 为页数量
bool vmm_unmap(pointer_t va, uint32_t size) {
    uint32_t pdeI = PDE_INDEX(va);
    uint32_t pteI = PTE_INDEX(va);
    for (; size > 0; pdeI++, pteI = 0) {
        pte_t *pte = &k_pde[pdeI];
        assertk((*pte & MM_PRES) == 0);
        for (; pteI < N_PDE; pteI++, size--)
            pte[pteI] = 0;
    }
    // TODO: 将被释放的空间添加到空闲列表
}


// size 为需要分配的虚拟内存页数量
void *vmm_alloc(uint32_t size) {
    void *addr = list_splitr(size);
    if (addr == MM_NULL)return MM_NULL;
    //TODO: 映射
    pointer_t phy_addr = phymm_alloc();
    if (!mm_map((pointer_t) addr, phy_addr | MM_KW | MM_PRES, size)) {
        phymm_free(phy_addr);
        return MM_NULL;
    };
    return addr;
}