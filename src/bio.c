//
// Created by pjs on 2021/4/7.
//
//磁盘缓冲块

#include "bio.h"
#include "drivers/ide.h"
#include "klib/qstring.h"

static struct cache {
    buf_t buf[N_BUF];
    buf_t head;
} cache;

void bio_init() {
    // 循环队列
    cache.head.next = &cache.head;
    for (buf_t *buf = cache.buf; buf < cache.buf + N_BUF; buf++) {
        buf->q_next = NULL;
        buf->next = buf < cache.buf + N_BUF ? &cache.head : buf + 1;
        buf->flag = 0;
        buf->ref_cnt = 0;
    }
}

buf_t *bio_get(uint32_t no_secs) {
    //用扇区号查找缓冲块
    for (buf_t *buf = cache.buf; buf < cache.buf + N_BUF; buf = buf->next) {
        if (buf->no_secs == no_secs) {
            buf->ref_cnt++;
            return buf;
        }
    }

    //没有查找到则回收利用没有被引用的缓冲块
    for (buf_t *buf = cache.buf; buf < cache.buf + N_BUF; buf = buf->next) {
        if (buf->ref_cnt == 0) {
            buf->no_secs = no_secs;
            buf->ref_cnt++;
            buf->flag = 0;
            return buf;
        }
    }

    //如果没有多余的缓冲块
    //TODO: 睡眠等待唤醒
}

void bio_read(buf_t *buf) {
    while (buf->flag & BUF_BSY) {
        // TODO: 阻塞线程
    }

    if (buf->flag & BUF_VALID) return;
    ide_rw(buf);
}

void bio_write(buf_t *buf, void *data) {
    while (buf->flag & BUF_BSY) {
        // TODO: 阻塞线程
    }
    q_memcpy(buf->data, data, SECTOR_SIZE);
    buf->flag |= (BUF_DIRTY | BUF_VALID);
    ide_rw(buf);
}


void bio_free(buf_t *_buf) {
    for (buf_t *buf = cache.buf; buf < cache.buf + N_BUF; buf = buf->next) {
        if (buf->no_secs == _buf->no_secs) {
            if (buf->ref_cnt >= 1) {
                buf->ref_cnt = 0;
            }
            return;
        }
    }
}