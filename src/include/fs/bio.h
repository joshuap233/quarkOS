//
// Created by pjs on 2021/4/7.
//
// 磁盘缓冲区

#ifndef QUARKOS_FS_BIO_H
#define QUARKOS_FS_BIO_H

#include "types.h"
#include "buf.h"

void bio_init();

void bio_write(buf_t *buf, void *data);

void bio_free(buf_t *_buf);

buf_t *bio_read_sync(uint32_t no_secs);

buf_t *bio_read(uint32_t no_secs);

#ifdef TEST

void test_ide_rw();

void test_dma_rw();

#endif // TEST

#endif //QUARKOS_FS_BIO_H
