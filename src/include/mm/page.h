//
// Created by pjs on 2021/5/30.
//

#ifndef QUARKOS_MM_PAGE_H
#define QUARKOS_MM_PAGE_H
#include <lib/rwlock.h>
#include <lib/queue.h>


struct page {
#define BUF_DIRTY   1
#define BUF_VALID   (1<<1)
#define SECTOR_SIZE 512
#define BUF_SIZE    PAGE_SIZE
    u32_t sizeLog: 8;                // 当前节点管理的内存单元(4K)个数取log2
    u32_t pn: 24;                    // 页帧号
    u16_t flag;
    u16_t ref_cnt;
    union {
        // TODO: 需要 address_space?
        // 分配大块内存时(分配的内存物理上可能并不连续), 使用 address_space 管理,
        // 这样一个 page 就只需要一个 address_space 结构指针,
        // 而不是 address_space ,节省内存,
        // 用于 buddy 管理
        struct buddy_page {
            struct page *left;
            struct page *right;
        } buddyPage;
        // 用于页缓存
        struct cache_page {
            list_head_t list;
            lfq_node dirty;
            u32_t timestamp;
            u32_t no_secs;
        } cachePage;
    };
    rwlock_t rwlock;
};

#endif //QUARKOS_MM_PAGE_H
