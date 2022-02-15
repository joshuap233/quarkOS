//
// Created by pjs on 2021/4/7.
//
// 磁盘缓冲区

#ifndef QUARKOS_FS_PAGE_CACHE_H
#define QUARKOS_FS_PAGE_CACHE_H

#include <types.h>
#include <mm/page.h>

#define WRITE_BACK_INTERVAL 5  // 刷入线程5s运行一次
#define MAX_DIRTY_PAGE      10 // 超过 10 个脏页唤醒刷入线程
#define CACHE_EXPIRES       30 // 页缓最近读写时间超过 30s 则可以回收

void mark_page_dirty(struct page*buf);

void page_read_sync(struct page*buf);

void page_read(struct page*buf);

void page_sync(struct page*buf);

struct page*page_get(uint32_t no_secs);

void page_recycle(u32_t size);

struct page*page_read_no(uint32_t no_secs);

struct page*page_read_no_sync(uint32_t no_secs);

void page_fsync();

#endif //QUARKOS_FS_PAGE_CACHE_H
