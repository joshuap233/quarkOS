//
// Created by pjs on 2021/4/7.
//
//磁盘缓冲块

#include "fs/bio.h"
#include "drivers/ide.h"
#include "lib/qstring.h"
#include "lib/list.h"
#include "sched/kthread.h"
#include "lib/qlib.h"

static struct cache {
    buf_t buf[N_BUF];
    spinlock_t lock;
    queue_t queue; // 睡眠等待缓的存块队列
} cache;

#define for_each_buf(buf) for ((buf) = &cache.buf[0]; (buf) < cache.buf + N_BUF; (buf)++)

void bio_init() {
    // 循环队列
    spinlock_init(&cache.lock);
    queue_init(&cache.queue);
    buf_t *buf;
    for_each_buf(buf) {
        queue_init(&buf->queue);
        buf->flag = 0;
        buf->ref_cnt = 0;
        spinlock_init(&buf->lock);
    }
}

static buf_t *bio_get(uint32_t no_secs) {
    //用扇区号查找缓冲块, 阻塞直到有可以缓冲块
    while (1) {
        spinlock_lock(&cache.lock);
        buf_t *buf;
        for_each_buf(buf) {
            if (buf->no_secs == no_secs) {
                buf->ref_cnt++;
                spinlock_unlock(&cache.lock);
                return buf;
            }
        }

        //没有查找到则回收利用没有被引用的缓冲块
        for_each_buf(buf) {
            if (buf->ref_cnt == 0) {
                buf->no_secs = no_secs;
                buf->ref_cnt++;
                buf->flag = 0;
                spinlock_unlock(&cache.lock);
                return buf;
            }
        }
        //如果没有多余的缓冲块, 则睡眠
        block_thread(&cache.queue, &cache.lock);
    }
}

void bio_read(uint32_t no_secs) {
    buf_t *buf = bio_get(no_secs);
    spinlock_lock(&buf->lock);
    if (buf->flag & BUF_VALID) {
        spinlock_lock(&buf->lock);
        return;
    };
    ide_rw(buf);
    block_thread(&buf->queue, &buf->lock);
}

void bio_write(buf_t *buf, void *data) {
    spinlock_lock(&buf->lock);
    q_memcpy(buf->data, data, SECTOR_SIZE);
    buf->flag |= (BUF_DIRTY | BUF_VALID);
    ide_rw(buf);
    block_thread(&buf->queue, &buf->lock);
}


void bio_free(buf_t *_buf) {
    assertk(_buf->ref_cnt > 0);
    buf_t *buf;
    for_each_buf(buf) {
        if (buf->no_secs == _buf->no_secs) {
            buf->ref_cnt--;
            if (buf->ref_cnt == 1 && !queue_empty(&cache.queue)) {
                //唤醒等待获取 buf 的线程
                unblock_thread(queue_head(&cache.queue));
            }
            return;
        }
    }
}