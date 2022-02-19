//
// Created by pjs on 2021/5/30.
//

#ifndef QUARKOS_MM_PAGE_H
#define QUARKOS_MM_PAGE_H

#include <lib/rwlock.h>
#include <lib/queue.h>
#include <lib/list.h>

// slab 块结构
typedef struct chunkLink {
    struct chunkLink *next;
} chunkLink_t;

// flag
#define PG_HEAD_BIT  0
#define PG_CACHE_BIT 5
#define PG_SLAB_BIT  6

// 页分配器 flag
#define PG_Head  1
#define PG_Tail  0

#define PG_Hole  0b00               // hold 页为物理内存空洞,不可用
#define PG_Page  (1<<1)

// 页缓存 flag,所有缓存页带有 PG_CACHE flag
#define PG_DIRTY (1<<2)
#define PG_VALID (1<<3)
#define PG_CACHE (1<<4)

// slab flag
#define PG_SLAB  (1<<5)

// slab 头信息
typedef struct slabInfo {
    chunkLink_t *chunk;     // 指向第一个可用内存块
    u16_t size;          // slab 内存块大小
    u16_t n_allocated;   // 已经分配块个数
#ifdef DEBUG
    u32_t magic;            // 用于 debug
#endif //DEBUG
} slabInfo_t;

// 页缓存
struct pageCache {
    lfq_node dirty;         // 脏页队列
    u32_t timestamp;        // 上次访问该页的时间
    u32_t no_secs;       // 需要读写的扇区起始 lba 值
};

#define SECTOR_SIZE 512
#define BUF_SIZE    PAGE_SIZE
#define PAGE_MAGIC  0x265ef118

struct page {
    list_head_t head;
    u32_t size;                    // 当前页管理的内存单元大小(byte)
    u16_t flag;
    u16_t ref_cnt;                 // 引用计数
    void *data;                    // 页对应的虚拟地址

#ifdef DEBUG
    u32_t magic;
#endif //DEBUG

    union {
        struct slabInfo slab;
        struct pageCache pageCache;
    };

    rwlock_t rwlock;
};

INLINE bool is_slab(struct page *page) {
    return page->flag & PG_SLAB;
}

INLINE bool page_head(struct page *page) {
    return page->flag & PG_Head;
}

#define PAGE_ENTRY(ptr)       list_entry(ptr,struct page,head)
#define SLAB_ENTRY(ptr)       (&PAGE_ENTRY(ptr)->slab)

INLINE struct pageCache *cache_entry(list_head_t *head) {
    return &PAGE_ENTRY(head)->pageCache;
}

INLINE struct pageCache *cache_dirty_entry(lfq_node *node) {
    return list_entry(node, struct pageCache, dirty);
}

INLINE struct page *page_dirty_entry(lfq_node *node) {
    struct pageCache *cache = cache_dirty_entry(node);
    return list_entry(cache, struct page, pageCache);
}

#endif //QUARKOS_MM_PAGE_H
