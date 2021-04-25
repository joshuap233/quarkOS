//
// Created by pjs on 2021/2/3.
//
// 内核堆
// TODO: realloc
#include "types.h"
#include "lib/list.h"
#include "mm/heap.h"
#include "mm/vmm.h"
#include "lib/qlib.h"

static heap_t heap;

#define cnk_entry(ptr) list_entry(ptr, heap_ptr_t, head)
#define HEAD heap.header
#define HDR_CHUNK cnk_entry(HEAD.next)
#define TAL_CHUNK cnk_entry(HEAD.prev)

//返回当前堆末尾地址
#define HEAP_TAIL (HEAP_START + heap.size - 1)

//计算剩余可用于扩展堆的虚拟内存
#define HEAP_VMM_FREE (HEAP_SIZE - heap.size)

INLINE void *alloc_addr(void *addr);

INLINE void *chunk_header(void *addr);

INLINE size_t chunk_size(size_t size);

static void hdr_init(heap_ptr_t *c, uint32_t size);

static bool expend(uint32_t size);

static void shrink();

void heap_init() {
    list_header_init(&HEAD);
    heap.size = PAGE_SIZE;

    //初始空闲块
    heap_ptr_t *hdr = (heap_ptr_t *) HEAP_START;
    vmm_mapv(HEAP_START, PAGE_SIZE, VM_KW | VM_PRES);
    hdr_init(hdr, PAGE_SIZE);

    list_add_next(&hdr->head, &HEAD);

#ifdef TEST
    test_mallocK_and_freeK();
    test_shrink_and_expand();
    test_allocK_page();
#endif //TEST
}


INLINE void *alloc_addr(void *addr) {
    //计算实际分配的内存块首地址
    return addr + sizeof(heap_ptr_t);
}

INLINE void *chunk_header(void *addr) {
    //addr 为需要释放的内存地址,返回包括头块的地址
    return addr - sizeof(heap_ptr_t);
}

INLINE size_t chunk_size(size_t size) {
    //size为需要分配的内存大小,返回包括头块的大小
    return size + sizeof(heap_ptr_t);
}

//初始化一个新头块
static void hdr_init(heap_ptr_t *c, uint32_t size) {
    c->size = size;
    c->used = false;
    c->magic = HEAP_MAGIC;
}

// size 为需要扩展的内存块大小
static bool expend(uint32_t size) {
    size = PAGE_ALIGN(size);
    if (HEAP_VMM_FREE < size) return false;

    vmm_mapv(HEAP_TAIL + 1, size, VM_KW | VM_PRES);
    if (TAL_CHUNK->used) {
        heap_ptr_t *new = (heap_ptr_t *) (HEAP_TAIL + 1);
        list_add_next(&new->head, &TAL_CHUNK->head);
        hdr_init(new, size);
    } else {
        TAL_CHUNK->size += size;
    }
    heap.size += size;
    return true;
}

static void shrink() {
    //保留需要释放的内存块所占用的第一页
    if (!TAL_CHUNK->used && TAL_CHUNK->size > HEAP_FREE_LIMIT) {
        ptr_t chunk_end = (ptr_t) TAL_CHUNK + sizeof(heap_ptr_t);

        void *addr = (void *) ((chunk_end & (~ALIGN_MASK)) + PAGE_SIZE);

        uint32_t size = addr - (void *) TAL_CHUNK;
        uint32_t free_size = TAL_CHUNK->size - size;
        assertk(!(free_size & ALIGN_MASK));

        TAL_CHUNK->size = size;
        heap.size -= free_size;
        vmm_unmap(addr, free_size);
    }
}

//合并内存块
static void merge(heap_ptr_t *alloc) {
    list_head_t *header = &alloc->head, *tail = &alloc->head;
    uint32_t size = alloc->size;
    //空头块始终为 used
    while (header->prev != &HEAD && (!cnk_entry(header->prev)->used)) {
        header = header->prev;
        size += cnk_entry(header)->size;
    }
    //向后合并
    while (tail->next != &HEAD && (!cnk_entry(tail->next)->used)) {
        tail = tail->next;
        size += cnk_entry(tail)->size;
    }

    if (header != tail) {
        list_link(header, tail->next);
        cnk_entry(header)->size = size;
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
        list_add_next(&new->head, &chunk->head);
        hdr_init(new, free_size);
    }
    chunk->used = true;
    chunk->size = size;
    return alloc_addr(chunk);
}

void *mallocK(size_t size) {
    size = MEM_ALIGN(chunk_size(size), HEAP_ALIGNMENT);

    list_for_each(&HEAD) {
        if (!cnk_entry(hdr)->used && cnk_entry(hdr)->size >= size)
            return alloc_chunk(cnk_entry(hdr), size);
    }

    if (!expend(TAL_CHUNK->used ? size : size - TAL_CHUNK->size)) return NULL;
    return alloc_chunk(TAL_CHUNK, size);
}

// 分配页对齐的一页(页内不包括头块)
void *allocK_page() {
    size_t size = chunk_size(PAGE_SIZE);
    if (!expend(TAL_CHUNK->used ? PAGE_SIZE * 2 : PAGE_SIZE)) return NULL;

    heap_ptr_t *new = (heap_ptr_t *) (HEAP_TAIL + 1 - size);
    hdr_init(new, size);
    new->used = true;

    TAL_CHUNK->size -= size;
    list_add_prev(&new->head, &HEAD);
    return alloc_addr(new);
}


// 分配对齐内存, align > 16 (mallocK 默认 16 字节对齐)
// align 为 4 的倍数
void *align_mallocK(u32_t size, u32_t align) {
    assertk(align > 16 && (align & 0x3) == 0);
    u32_t offset = align - 1 + sizeof(void *);
    void *p1 = (void *) mallocK(size + offset);
    if (p1 == NULL) return NULL;
    void **p2 = (void **) (((ptr_t) p1 + offset) & (~(align - 1)));
    p2[-1] = p1;
    return p2;
}

void *align_freeK(void *addr) {
    void *p = ((void **) addr)[-1];
    freeK(p);
}

void freeK(void *addr) {
    heap_ptr_t *alloc = chunk_header(addr);
    assertk(alloc->magic == HEAP_MAGIC);
    alloc->used = false;
    merge(alloc);
    shrink();
}

// 初始化固定块分配器, size 为需要预先分配的块数
void chunk_init(blkAlloc_t *alloc, u32_t blockSize, u32_t size, u32_t align) {
    alloc->blockSize = blockSize;
    alloc->top = 1;
    alloc->size = size;

}

void *chunk_alloc(blkAlloc_t *allocator, size_t size) {

}

void chunk_free(blkAlloc_t *allocator, void *addr) {

}


//=============== 测试 ================

#ifdef TEST

static size_t get_unused_space() {
    size_t size = 0;
    list_for_each(&HEAD) {
        if (!cnk_entry(hdr)->used) size += cnk_entry(hdr)->size;
    }
    return size;
}

static size_t get_used_space() {
    size_t size = 0;
    list_for_each(&HEAD) {
        if (cnk_entry(hdr)->used) size += cnk_entry(hdr)->size;
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
    assertk(HDR_CHUNK->size == heap.size);
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
    assertk(HDR_CHUNK->size == heap.size);
    test_pass;
}

void test_allocK_page() {
    test_start;
    size_t unused = get_unused_space();
    size_t used = get_used_space();
    void *addr[3];

    addr[0] = allocK_page();
    assertk(addr[0] != NULL);
    assertk(!((ptr_t) addr[0] & ALIGN_MASK));

    addr[1] = allocK_page();
    assertk(addr[1] != NULL);
    assertk(!((ptr_t) addr[1] & ALIGN_MASK));

    addr[2] = allocK_page();
    assertk(addr[2] != NULL);
    assertk(!((ptr_t) addr[2] & ALIGN_MASK));

    freeK(addr[0]);
    assertk(get_unused_space() == PAGE_SIZE * 3 + unused - sizeof(heap_ptr_t) * 2);
    freeK(addr[1]);
    assertk(get_unused_space() == PAGE_SIZE * 4 + unused - sizeof(heap_ptr_t));
    freeK(addr[2]);
    assertk(unused == get_unused_space());
    assertk(used == get_used_space());
    assertk(HDR_CHUNK->size == heap.size);
    test_pass;
}

#endif //TEST