//
// Created by pjs on 2021/2/3.
//
// 内核堆
#include "mm/heap.h"
#include "types.h"
#include "mm/mm.h"
#include "mm/virtual_mm.h"
#include "klib/qmath.h"

static heap_t heap;


//返回当前堆末尾地址
#define HEAP_TAIL (HEAP_START + heap.size - 1)

//计算剩余可用于扩展堆的虚拟内存
#define HEAP_VMM_FREE (HEAP_SIZE - heap.size)

__attribute__((always_inline))
static inline void *alloc_addr(void *addr) {
    //计算实际分配的内存块首地址
    return addr + sizeof(heap_ptr_t);
}

__attribute__((always_inline))
static inline void *chunk_header(void *addr) {
    //addr 为需要释放的内存地址,返回包括头块的地址
    return addr - sizeof(heap_ptr_t);
}

__attribute__((always_inline))
static inline size_t chunk_size(size_t size) {
    //size为需要分配的内存大小,返回包括头块的大小
    return size + sizeof(heap_ptr_t);
}

__attribute__((always_inline))
static inline void _list_add(heap_ptr_t *new, heap_ptr_t *prev, heap_ptr_t *next) {
    new->prev = prev;
    new->next = next;
    next->prev = new;
    prev->next = new;
}

// new 节点添加到 target 后
__attribute__((always_inline))
static inline void _list_add_next(heap_ptr_t *new, heap_ptr_t *target) {
    _list_add(new, target, target->next);
}

__attribute__((always_inline))
static inline void _list_add_prev(heap_ptr_t *new, heap_ptr_t *target) {
    _list_add(new, target->prev, target);
}


__attribute__((always_inline))
static inline void _list_link(heap_ptr_t *header, heap_ptr_t *tail) {
    header->next = tail;
    tail->prev = header;
}


__attribute__((always_inline))
static inline void list_header_init(heap_ptr_t *header) {
    header->next = header;
    header->prev = header;
}


//初始化一个新头块
static void chunk_init(heap_ptr_t *c, uint32_t size) {
    c->size = size;
    c->used = false;
    c->magic = HEAP_MAGIC;
}

void heap_init() {
    heap.size = PAGE_SIZE;
    heap.header = (heap_ptr_t *) HEAP_START;
    vmm_mapv((pointer_t) heap.header, heap.size, VM_KW | VM_PRES);
    //初始化空头块
    list_header_init(heap.header);
    chunk_init(heap.header, sizeof(heap_ptr_t));
    heap.header->used = true;

    // 初始空闲块
    heap_ptr_t *new = heap.header + 1;
    chunk_init(new, PAGE_SIZE - heap.header->size);
    _list_add_next(new, heap.header);

    test_mallocK_and_freeK();
    test_shrink_and_expand();
    test_allocK_page();

}

// size 为需要扩展的内存块大小
static bool expend(uint32_t size) {
    size = SIZE_ALIGN(size);
    if (HEAP_VMM_FREE < size) return false;
    heap_ptr_t *tail = heap.header->prev;
    vmm_mapv(HEAP_TAIL + 1, size, VM_KW | VM_PRES);
    if (tail->used) {
        heap_ptr_t *new = (heap_ptr_t *) (HEAP_TAIL + 1);
        _list_add_next(new, tail);
        chunk_init(new, size);
    } else {
        tail->size += size;
    }
    heap.size += size;
    return true;
}

static inline void shrink() {
    heap_ptr_t *tail = heap.header->prev;

    //保留需要释放的内存块所占用的第一页
    if (!tail->used && tail->size > HEAP_FREE_LIMIT) {
        pointer_t chunk_header_end = (pointer_t) tail + sizeof(heap_ptr_t);

        void *addr = (void *) ((chunk_header_end & (~ALIGN_MASK)) + PAGE_SIZE);

        uint32_t size = addr - (void *) tail;
        uint32_t free_size = tail->size - size;
        assertk(!(free_size & ALIGN_MASK));

        tail->size = size;
        heap.size -= free_size;
        vmm_unmap(addr, free_size);
    }
}

//合并内存块
static void merge(heap_ptr_t *alloc) {
    heap_ptr_t *header = alloc, *tail = alloc;
    uint32_t size = alloc->size;
    //空头块始终为 used
    while (!header->prev->used) {
        header = header->prev;
        size += header->size;
    }
    //向后合并
    while (!tail->next->used) {
        tail = tail->next;
        size += tail->size;
    }

    if (header != tail) {
        _list_link(header, tail->next);
        header->size = size;
    }
}

// chunk 为可用空闲块, size 为需要分割的大小
static void *alloc_chunk(heap_ptr_t *chunk, size_t size) {
    assertk(!chunk->used);
    size_t free_size = chunk->size - size;
    if (free_size < sizeof(heap_ptr_t)) {
        //剩余空间无法容纳新头块,将剩余空间全都分配出去
        size = chunk->size;
    } else {
        heap_ptr_t *new = (void *) chunk + size;
        _list_add_next(new, chunk);
        chunk_init(new, free_size);
    }
    chunk->used = true;
    chunk->size = size;
    return alloc_addr(chunk);
}

void *mallocK(size_t size) {
    size = chunk_size(size);

    for (heap_ptr_t *hdr = heap.header->next; hdr != heap.header; hdr = hdr->next) {
        if (!hdr->used && hdr->size >= size)
            return alloc_chunk(hdr, size);
    }

    heap_ptr_t *tail = heap.header->prev;
    if (!expend(tail->used ? size : size - tail->size)) return NULL;
    // expand 后 tail 可能已经不指向最后一块,因此使用  heap.header->prev 访问
    return alloc_chunk(heap.header->prev, size);
}

// 分配页对齐的一页(页内不包括头块)
void *allocK_page() {
    size_t size = chunk_size(PAGE_SIZE);
    if (!expend(heap.header->prev->used ? PAGE_SIZE * 2 : PAGE_SIZE)) return NULL;

    heap_ptr_t *new = (heap_ptr_t *) (HEAP_TAIL + 1 - size);
    chunk_init(new, size);
    new->used = true;

    heap.header->prev->size -= size;
    _list_add_prev(new, heap.header);
    return alloc_addr(new);
}


void freeK(void *addr) {
    heap_ptr_t *alloc = chunk_header(addr);
    assertk(alloc->magic == HEAP_MAGIC);
    alloc->used = false;
    merge(alloc);
    shrink();
}

//=============== 测试 ================

static size_t get_unused_space() {
    size_t size = 0;
    for (heap_ptr_t *hdr = heap.header->next; hdr != heap.header; hdr = hdr->next) {
        if (!hdr->used) size += hdr->size;
    }
    return size;
}

static size_t get_used_space() {
    size_t size = 0;
    for (heap_ptr_t *hdr = heap.header->next; hdr != heap.header; hdr = hdr->next) {
        if (hdr->used) size += hdr->size;
    }
    return size;
}


void test_mallocK_and_freeK() {
    test_start;
    size_t unused = get_unused_space();
    size_t used = get_used_space();
    void *addr[3];

    addr[0] = mallocK(20);
    assertk(addr[0] != NULL);

    addr[1] = mallocK(40);
    assertk(addr[1] != NULL);

    addr[2] = mallocK(30);
    assertk(addr[2] != NULL);
    assertk(get_unused_space() == unused - (3 * sizeof(heap_ptr_t) + 20 + 40 + 30));

    freeK(addr[0]);
    assertk(get_unused_space() == unused - (2 * sizeof(heap_ptr_t) + 40 + 30));
    freeK(addr[1]);
    assertk(get_unused_space() == unused - (sizeof(heap_ptr_t) + 30));
    freeK(addr[2]);
    assertk(unused == get_unused_space());
    assertk(used == get_used_space());
    assertk(heap.header->size + heap.header->next->size == heap.size);
    test_pass;
}


void test_shrink_and_expand() {
    test_start;
    size_t unused = get_unused_space();
    size_t used = get_used_space();
    void *addr[3];

    addr[0] = mallocK(PAGE_SIZE);
    assertk(get_unused_space() == unused - sizeof(heap_ptr_t));
    assertk(addr[0] != NULL);

    addr[1] = mallocK(PAGE_SIZE);
    assertk(addr[1] != NULL);
    assertk(get_unused_space() == unused - sizeof(heap_ptr_t) * 2);

    addr[2] = mallocK(PAGE_SIZE);
    assertk(addr[2] != NULL);
    assertk(get_unused_space() == unused - sizeof(heap_ptr_t) * 3);

    freeK(addr[0]);
    freeK(addr[1]);
    freeK(addr[2]);

    assertk(unused == get_unused_space());
    assertk(used == get_used_space());
    assertk(heap.header->size + heap.header->next->size == heap.size);
    test_pass;
}

void test_allocK_page() {
    test_start;
    size_t unused = get_unused_space();
    size_t used = get_used_space();
    void *addr[3];

    addr[0] = allocK_page();
    assertk(addr[0] != NULL);
    assertk(!((pointer_t) addr[0] & ALIGN_MASK));

    addr[1] = allocK_page();
    assertk(addr[1] != NULL);
    assertk(!((pointer_t) addr[1] & ALIGN_MASK));

    addr[2] = allocK_page();
    assertk(addr[2] != NULL);
    assertk(!((pointer_t) addr[2] & ALIGN_MASK));

    freeK(addr[0]);
    assertk(get_unused_space() == PAGE_SIZE * 3 + unused - sizeof(heap_ptr_t) * 2);
    freeK(addr[1]);
    assertk(get_unused_space() == PAGE_SIZE * 4 + unused - sizeof(heap_ptr_t));
    freeK(addr[2]);
    assertk(unused == get_unused_space());
    assertk(used == get_used_space());
    assertk(heap.header->size + heap.header->next->size == heap.size);
    test_pass;
}
