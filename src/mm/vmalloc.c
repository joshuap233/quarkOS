//
// Created by pjs on 2021/5/30.
//
// 管理用户空间虚拟内存
#include <mm/vmalloc.h>
#include <mm/mm.h>
#include <mm/kvm.h>
#include <mm/page_alloc.h>
#include <lib/qlib.h>
#include <lib/list.h>

#define area_entry(ptr) list_entry(ptr,struct vm_area,head)


int32_t vm_area_insert(mm_struct_t *mm, struct vm_area *area) {
    list_head_t *hdr;
    struct vm_area *entry;
    list_for_each(hdr, &mm->area) {
        entry = area_entry(hdr);
        if (area->addr < entry->addr) break;
    }
    if (hdr != &mm->area) {
        entry = area_entry(hdr);
        if (entry->addr < area->addr + area->size) {
            return -1;
        }
    }
    list_add_prev(&area->head, hdr);
    return 0;
}


static void mm_struct_init(mm_struct_t *mm,
                           ptr_t txt, ptr_t size1,
                           ptr_t rodata, ptr_t size2,
                           ptr_t dataBss, ptr_t size3) {
    assertk(txt + size1 < 3 * G);
    assertk(rodata + size2 < 3 * G);
    assertk(dataBss + size3 < 3 * G - PAGE_SIZE);
    size1 = PAGE_ALIGN(size1);
    size2 = PAGE_ALIGN(size2);
    size3 = PAGE_ALIGN(size3);

    list_header_init(&mm->area);
    mm->_area[0].addr = txt;
    mm->_area[0].size = size1;
    mm->_area[0].flag = VM_PRES | VM_UR;
    list_add_prev(&mm->_area[0].head, &mm->area);

    mm->_area[1].addr = rodata;
    mm->_area[1].size = size2;
    mm->_area[1].flag = VM_PRES | VM_UR;
    assertk(vm_area_insert(mm, &mm->_area[1]) == 0);

    mm->_area[2].addr = dataBss;
    mm->_area[2].size = size3;
    mm->_area[2].flag = VM_PRES | VM_UW;
    assertk(vm_area_insert(mm, &mm->_area[2]) == 0);

    mm->_area[3].addr = dataBss + PAGE_SIZE;
    mm->_area[3].size = 0;
    mm->_area[3].flag = VM_PRES | VM_UW;
    assertk(vm_area_insert(mm, &mm->_area[3]) == 0);

    mm->brk = &mm->_area[3];
    mm->size = size1 + size2 + size3;
}


//ptr_t vm_area_alloc(mm_struct_t *mm, u32_t size, u16_t flag) {
//    // 从 brk 向后查找
//    size = PAGE_ALIGN(size);
//    list_head_t *hdr;
//    list_for_each(hdr, &mm->area) {
//
//    }
//}
//
//int32_t vm_area_del(ptr_t addr) {
//    // 只能删除整个 vm_area,不能切割内存
//}


//ptr_t vm_init(mm_struct_t *mm) {
//    ptr_t pgdir = pm_alloc_page();
//    return pgdir;
//}
//
//void vm_map_page(mm_struct_t *mm) {
//
//}
//
//void vm_maps() {
//
//}
//
//
//void vm_unmap_page() {
//
//}
//
//void vm_unmaps() {
//
//}
//
//void vm_vm2pm() {
//
//}