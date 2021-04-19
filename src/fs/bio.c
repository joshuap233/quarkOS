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
#include "drivers/dma.h"

static struct cache {
    buf_t buf[N_BUF];
    spinlock_t lock;
    queue_t sleep; // 睡眠等待缓存块队列
} cache;

#define for_each_buf(buf) for ((buf) = &cache.buf[0]; (buf) < cache.buf + N_BUF; (buf)++)

static struct disk {
    void (*rw)(buf_t *);

    void (*isr)(UNUSED interrupt_frame_t *frame);
} disk;

INT disk_isr(UNUSED interrupt_frame_t *frame);

void bio_init() {
    // 循环队列
    spinlock_init(&cache.lock);
    queue_init(&cache.sleep);
    buf_t *buf;
    for_each_buf(buf) {
        queue_init(&buf->queue);
        queue_init(&buf->sleep);
        buf->flag = 0;
        buf->ref_cnt = 0;
        spinlock_init(&buf->lock);
    }
    disk.rw = dma_dev.dma ? dma_rw : ide_rw;
    disk.isr = dma_dev.dma ? dma_isr_handler : ide_isr_handler;
    reg_isr(46, disk_isr);
}

static buf_t *bio_get(uint32_t no_secs) {
    buf_t *temp = NULL;
    //用扇区号查找缓冲块, 阻塞直到有可以缓冲块
    spinlock_lock(&cache.lock);
    while (1) {
        buf_t *buf;
        for_each_buf(buf) {
            if (buf->no_secs == no_secs) {
                buf->ref_cnt++;
                spinlock_unlock(&cache.lock);
                return buf;
            }
            if (!temp && buf->ref_cnt == 0) {
                // 可以被回收的缓存块
                temp = buf;
            }
        }

        // 没有找多空闲缓存块则使用没有被引用的缓存块
        if (temp) {
            temp->no_secs = no_secs;
            temp->ref_cnt++;
            temp->flag = 0;
            spinlock_unlock(&cache.lock);
            return temp;
        }

        block_thread(&cache.sleep, &cache.lock);
    }
}

// 强制重新读取磁盘
buf_t *bio_read_sync(uint32_t no_secs) {
    buf_t *buf = bio_get(no_secs);
    spinlock_lock(&buf->lock);

    disk.rw(buf);
    block_thread(&buf->sleep, &buf->lock);

    spinlock_unlock(&buf->lock);
    return buf;
}

// sync 被设置则强制重新读取磁盘
buf_t *bio_read(uint32_t no_secs) {
    buf_t *buf = bio_get(no_secs);
    spinlock_lock(&buf->lock);

    if ((buf->flag & (BUF_VALID | BUF_BSY)) == BUF_VALID) {
        spinlock_unlock(&buf->lock);
        return buf;
    }

    if (!(buf->flag & BUF_VALID)) {
        disk.rw(buf);
    }

    // 可能在 block_thread 调用前,磁盘中断处理程序以及被调用
    // 中断处理程序会释放 buf->lock
    if (block_thread(&buf->sleep, &buf->lock) == 0) {
        spinlock_unlock(&buf->lock);
    }
    return buf;
}

void bio_write(buf_t *buf, void *data) {
    spinlock_lock(&buf->lock);
    if (buf->flag & BUF_BSY) {
        block_thread(&buf->sleep, &buf->lock);
    }

    q_memcpy(buf->data, data, SECTOR_SIZE);
    buf->flag |= (BUF_DIRTY | BUF_VALID);
    disk.rw(buf);

    if (block_thread(&buf->sleep, &buf->lock) == 0) {
        spinlock_unlock(&buf->lock);
    }
}


void bio_free(buf_t *_buf) {
    assertk(_buf->ref_cnt > 0);
    buf_t *buf;
    for_each_buf(buf) {
        if (buf->no_secs == _buf->no_secs && (--buf->ref_cnt) == 0 && !queue_empty(&cache.sleep)) {
            //唤醒等待获取 buf 的线程
            unblock_thread(queue_head(&cache.sleep));
            return;
        }
    }
}

INT disk_isr(UNUSED interrupt_frame_t *frame) {
    disk.isr(frame);
    pic2_eoi(46);
}

// ============ test =========
#ifdef TEST

void test_ide_rw() {
    test_start

    disk.rw = ide_rw;
    reg_isr(46, disk_isr);

    uint8_t data[SECTOR_SIZE];
    q_memset(data, 1, SECTOR_SIZE);

    buf_t *buf = bio_read(10);
    bio_write(buf, data);

    buf = bio_read_sync(10);
    q_memcmp(data, buf->data, SECTOR_SIZE);
    bio_free(buf);

    test_pass
}

void test_dma_rw() {
    disk.rw = dma_rw;
    reg_isr(46, disk_isr);
    assertk(dma_dev.dma);
}


#endif // TEST