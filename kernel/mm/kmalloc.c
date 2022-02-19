//
// Created by pjs on 2021/5/4.
//
#include <mm/kmalloc.h>
#include <mm/page_alloc.h>
#include <mm/slab.h>
#include <types.h>
#include <mm/vm.h>
#include <lib/qstring.h>
#include <lib/qlib.h>

void *kmalloc(u32_t size) {
    size = fixSize(size);
    if (size <= SLAB_MAX)
        return slab_alloc(size);
    struct page *page = __alloc_page(size);
    kvm_map(page, VM_PRES | VM_KRW);
    return page->data;
}

void *kcalloc(u32_t size) {
    size = fixSize(size);
    void *addr = kmalloc(size);
    bzero(addr, size);
    return addr;
}


void *krealloc(void *_addr, size_t _size) {
    void *addr = kmalloc(_size);
    assertk(addr);

    struct page *page = va_get_page(PAGE_ADDR((ptr_t) _addr));
    assertk(page);

    u32_t size = page->size;
    assertk(page->size < _size);

    if (is_slab(page))
        size = slab_chunk_size(addr);
    memcpy(addr, _addr, size);
    kfree(_addr);
    return addr;
}

// addr 为虚拟地址
void kfree(void *addr) {
    struct page *page = va_get_page(PAGE_ADDR((ptr_t) addr));
    assertk(page);

    if (is_slab(page))
        return slab_free(addr);
    kvm_unmap(page);
    __free_page(page);
}