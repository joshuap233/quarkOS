//
// Created by pjs on 2022/2/22.
//

#include <lib/list.h>
#include "malloc.h"
#include "lib.h"

typedef struct chunk_ptr {
    struct chunk_ptr *next;
    u32_t size;     //当前内存块长度,包括头块
    u32_t used: 1;  //当前内存块是否被使用
    u32_t magic: 31;
#define HEAP_MAGIC 0x35e92b2e
} chunk_t;

#define ALIGN(s, align) (((s)+(align)-1)&(~((align)-1)))

static struct chunk_ptr *head = NULL;
static struct chunk_ptr *tail = NULL;

INLINE void *alloc_addr(void *addr) {
    //计算实际分配的内存块首地址
    return addr + sizeof(chunk_t);
}

INLINE void *chunk_header(void *addr) {
    //addr 为需要释放的内存地址,返回包括头块的地址
    return addr - sizeof(chunk_t);
}


static void chunk_init(chunk_t *new, chunk_t *next, u32_t size){
    new->next = next;
    new->used = false;
    new->magic = HEAP_MAGIC;
    new->size = size;
}

static void *alloc_chunk(chunk_t *chunk,u32_t size){
    assert(!chunk->used && chunk->magic == HEAP_MAGIC);

    size_t free_size = chunk->size - size;
    if (free_size < (sizeof(chunk_t)+2)) {
        size = chunk->size;
    } else {
        chunk_t *new = (void *) chunk + size;
        chunk_init(new,chunk->next,free_size);
        chunk->next = new;
        if (chunk == tail)
            tail = new;
    }
    chunk->used = true;
    chunk->size = size;
    return alloc_addr(chunk);
}

void *malloc(size_t size) {
    // 分配的内存 2 字节对齐
    size = ALIGN(size + sizeof(struct chunk_ptr),2);
    for (chunk_t *hdr = head; hdr ; hdr=hdr->next) {
        if (!hdr->used && hdr->size >= size){
            return alloc_chunk(hdr,size);
        }
    }

    struct chunk_ptr * b = sbrk(4096);
    assert(b);
    if(!head) head = b;
    chunk_init(b,NULL,4096);
    tail = b;
    return alloc_chunk(b,size);
}


void free(void *addr) {
    chunk_t *chunk = chunk_header(addr);
    assert(chunk->used && chunk->magic == HEAP_MAGIC);

    chunk->used = false;
    // 向后合并
    chunk_t *p = chunk->next;
    u32_t size = chunk->size;
    for (;p && !p->used;p=p->next){
        assert(chunk->magic == HEAP_MAGIC);
        size += p->size;
    }
    if (p!=chunk->next){
        chunk->next = p;
        chunk->size = size;
        if (!p){
            tail = chunk;
        }
    }
}

//=============== 测试 ================
#ifdef TEST

static u32_t free_space(){
    u32_t size = 0;
    for (chunk_t *hdr = head; hdr ; hdr=hdr->next) {
        size += hdr->size;
    }
    return size;
}

void test_malloc(){
    test_start;
    for (int i = 0; i < 10; ++i) {
        void *p = malloc(i);
        free(p);
    }
    assert(free_space() == 4096);
    test_pass;
}


#endif //TEST
