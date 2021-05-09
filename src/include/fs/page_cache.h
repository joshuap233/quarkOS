//
// Created by pjs on 2021/4/7.
//
// 磁盘缓冲区

#ifndef QUARKOS_FS_PAGE_CACHE_H
#define QUARKOS_FS_PAGE_CACHE_H

#include "types.h"
#include "buf.h"

#define WRITE_BACK_INTERVAL 5  // 刷入线程5s运行一次
#define MAX_DIRTY_PAGE      10 // 超过 10 个脏页唤醒刷入线程
#define CACHE_EXPIRES       30 // 页缓最近读写时间超过 30s 则可以回收

void page_write(buf_t *buf, void *data);

buf_t *page_read_sync(buf_t *buf);

buf_t *page_read(buf_t *buf);

buf_t *page_get(uint32_t no_secs);

void page_write_sync(buf_t *buf, void *data);

void page_recycle();

#endif //QUARKOS_FS_PAGE_CACHE_H
