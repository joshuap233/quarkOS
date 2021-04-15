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
void bio_read(uint32_t no_secs);

#endif //QUARKOS_FS_BIO_H
