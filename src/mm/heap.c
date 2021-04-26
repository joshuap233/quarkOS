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
    test_align_mallocK_and_freeK();
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


void freeK(void *addr) {
    heap_ptr_t *alloc = chunk_header(addr);
    assertk(alloc->magic == HEAP_MAGIC);
    assertk(alloc->used == true);
    alloc->used = false;
    merge(alloc);
    shrink();
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

void align_freeK(void *addr) {
    void *p = ((void **) addr)[-1];
    freeK(p);
}


/*
 * 初始化固定块分配器, size 为需要预先分配的块数
 * align:     align 字节对齐, align > 16,不需要对齐则 align 为 0
 * blockSize: 固定块大小
 * num:       需要预先分配的块数(管理器自动扩展时会扩展 size 块)
 */
#define MEM_START(alloc, data) ((void*)((data)->stack) - (alloc)->blockSize * (alloc)->size)

void blkAlloc_init(blkAlloc_t *alloc, u32_t blockSize, u32_t num, u32_t align) {
    struct blkData *data = &alloc->data;
    u32_t total = (blockSize + sizeof(void *)) * num;
    void *mem = align ? align_mallocK(total, align) : mallocK(total);
    assertk(mem != NULL);

    alloc->align = align;
    alloc->blockSize = blockSize;
    alloc->size = num;

    data->next = NULL;
    data->top = num;
    data->stack = mem + blockSize * num;
    for (u32_t i = 0; i < num; ++i) {
        data->stack[i] = mem;
        mem += blockSize;
    }
}

// 释放 prev 后的块
static void blK_shrink(blkAlloc_t *alloc, struct blkData *prev) {
    struct blkData *dst = prev->next;
    if (dst != NULL && prev->top >= alloc->size / 2 && dst->top == alloc->size) {
        void *memStart = MEM_START(alloc, dst);
        alloc->align ? align_freeK(memStart) : freeK(memStart);
        prev->next = NULL;
    }
}

static void blK_expend(blkAlloc_t *alloc) {
    u32_t size = (alloc->blockSize + sizeof(void *)) * alloc->size;
    u32_t total = size + sizeof(struct blkData);
    u32_t align = alloc->align;
    void *mem = align ? align_mallocK(total, alloc->align) : mallocK(total);
    assertk(mem != NULL);

    alloc->data.next = mem + size;
    struct blkData *data = alloc->data.next;
    data->stack = mem + alloc->blockSize * alloc->size;
    data->next = NULL;
    data->top = alloc->size;
    for (u32_t i = 0; i < alloc->size; ++i) {
        data->stack[i] = mem;
        mem += alloc->blockSize;
    }
}

void *blk_alloc(blkAlloc_t *alloc) {
    struct blkData *data = &alloc->data;
    while (data->top == 0) {
        if (data->next != NULL)
            data = data->next;
        else
            blK_expend(alloc);
    };
    return data->stack[--data->top];
}

void blk_free(blkAlloc_t *alloc, void *addr) {
    if (alloc->align) {assertk(((ptr_t) addr & (alloc->align - 1)) == 0)};
    struct blkData *data = &alloc->data;
    for (; data != NULL && data->top != alloc->size; data = data->next) {
        void *memStart = MEM_START(alloc, data);
        if (addr >= memStart && addr < (void *) data->stack) {
            data->stack[data->top++] = addr;
            blK_shrink(alloc, data);
            return;
        }
    }
    assertk(0);
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

static size_t realSize(uint32_t size) {
    return MEM_ALIGN(size + sizeof(heap_ptr_t), HEAP_ALIGNMENT);
}

static size_t get_used_space() {
    size_t size = 0;
    list_for_each(&HEAD) {
        if (cnk_entry(hdr)->used) size += cnk_entry(hdr)->size;
    }
    return size;
}

size_t unused, used;
void *addr[3];

void test_mallocK_and_freeK() {
    test_start;
    unused = get_unused_space();
    used = get_used_space();
    size_t size[3] = {20, 40, 30};
    for (int i = 0; i < 3; ++i) {
        addr[i] = mallocK(size[i]);
        assertk(addr[i] != NULL);
        assertk(((ptr_t) addr[i] & (HEAP_ALIGNMENT - 1)) == 0)
    }

    for (int i = 0; i < 3; ++i) {
        u32_t temp = 0;
        for (int j = i; j < 3; ++j) {
            temp += realSize(size[j]);
        }
        assertk(get_unused_space() == unused - temp);
        freeK(addr[i]);
    }
    assertk(unused == get_unused_space());
    assertk(used == get_used_space());
    assertk(HDR_CHUNK->size == heap.size);
    test_pass;
}


void test_shrink_and_expand() {
    test_start;
    for (int i = 0; i < 3; ++i) {
        addr[i] = mallocK(PAGE_SIZE);
        assertk(get_unused_space() == unused - sizeof(heap_ptr_t) * (i + 1));
        assertk(addr[i] != NULL);
    }

    for (int i = 0; i < 3; ++i) {
        freeK(addr[i]);
    }
    assertk(unused == get_unused_space());
    assertk(used == get_used_space());
    assertk(HDR_CHUNK->size == heap.size);
    test_pass;
}

void test_align_mallocK_and_freeK() {
    test_start;
    size_t size[3] = {20, 40, 30};
    for (int i = 0; i < 3; ++i) {
        u32_t align = HEAP_ALIGNMENT * (i + 2);
        addr[i] = align_mallocK(size[i], align);
        assertk(addr[i] != NULL);
        assertk(((ptr_t) addr[i] & (align - 1)) == 0)
    }

    for (int i = 0; i < 3; ++i) {
        u32_t temp = 0;
        for (int j = i; j < 3; ++j) {
            u32_t align = HEAP_ALIGNMENT * (j + 2);
            temp += realSize(size[j] + align - 1 + sizeof(void *));
        }
        assertk(get_unused_space() == unused - temp);
        align_freeK(addr[i]);
    }
    assertk(unused == get_unused_space());
    assertk(used == get_used_space());
    assertk(HDR_CHUNK->size == heap.size);
    test_pass;
}

#endif //TEST