//
// Created by pjs on 2021/5/4.
//
#include <mm/kmalloc.h>
#include <mm/page_alloc.h>
#include <mm/slab.h>
#include <types.h>
#include <mm/kvm.h>
#include <lib/qstring.h>
#include <lib/qlib.h>

void *kmalloc(u32_t size) {
    size = fixSize(size);
    if (size <= SLAB_MAX)
        return slab_alloc(size);
    struct page *page = __alloc_page(size);
    kvm_map(page, VM_PRES | VM_KW);
    return page->data;
}

void *kcalloc(u32_t size) {
    size = fixSize(size);
    void *addr = kmalloc(size);
    q_memset(addr, 0, size);
    return addr;
}


void *krealloc(void *_addr, size_t _size) {
    void *addr = kmalloc(_size);
    assertk(addr);

    struct page *page = va_get_page(PAGE_ADDR((ptr_t) _addr));
    u32_t size = page->size;
    assertk(page->size < _size);

    if (is_slab(page))
        size = slab_chunk_size(addr);
    q_memcpy(addr, _addr, size);
    kfree(_addr);
    return addr;
}

void kfree(void *addr) {
    struct page *page = va_get_page(PAGE_ADDR((ptr_t) addr));

    if (is_slab(page))
        return slab_free(addr);
    kvm_unmap(page);
    __free_page(page);
}