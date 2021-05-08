//
// Created by pjs on 2021/4/7.
//
// 磁盘缓冲区

#ifndef QUARKOS_FS_PAGE_CACHE_H
#define QUARKOS_FS_PAGE_CACHE_H

#include "types.h"
#include "buf.h"

#define WRITE_BACK_INTERVAL 5

void bio_write(buf_t *buf, void *data);

void bio_free(buf_t *_buf);

buf_t *bio_read_sync(buf_t *buf);

buf_t *bio_read(buf_t *buf);

buf_t *bio_get(uint32_t no_secs);


#endif //QUARKOS_FS_PAGE_CACHE_H
