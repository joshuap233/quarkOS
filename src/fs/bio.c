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

//ata dma 要求缓冲区不能跨域 64K 边界
//且存入 prd 的地址为物理地址,
static _Alignas(BUF_SIZE) uint8_t buf_data[N_BUF][BUF_SIZE];

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
    uint8_t i = 0;
    for_each_buf(buf) {
        queue_init(&buf->queue);
        queue_init(&buf->sleep);
        buf->data = buf_data[i++];
        buf->flag = 0;
        buf->ref_cnt = 0;
        spinlock_init(&buf->lock);
    }
    disk.rw = ide_rw;
    disk.isr = ide_isr_handler;
//    disk.rw = dma_dev.dma ? dma_rw : ide_rw;
//    disk.isr = dma_dev.dma ? dma_isr_handler : ide_isr_handler;
    reg_isr(32 + 14, disk_isr);
}


buf_t *bio_get(uint32_t no_secs) {
    buf_t *unref = NULL;

    //用扇区号查找缓冲块, 阻塞直到有可以缓冲块
    spinlock_lock(&cache.lock);
    while (1) {
        buf_t *buf;
        for_each_buf(buf) {
            if (buf->no_secs == no_secs) {
                buf->ref_cnt++;
                spinlock_unlock(&cache.lock);
                spinlock_unlock(&buf->lock);
                return buf;
            }
            if (!unref && buf->ref_cnt == 0)
                unref = buf;
        }

        if (unref) {
            unref->flag = 0;
            unref->no_secs = no_secs;
            unref->ref_cnt++;
            spinlock_unlock(&cache.lock);
            return unref;
        }
        block_thread(&cache.sleep, &cache.lock);
    }
}


// 不适用缓冲区数据,强制重新读取磁盘
buf_t *bio_read_sync(buf_t *buf) {
    spinlock_lock(&buf->lock);

    disk.rw(buf);
    if (block_thread(&buf->sleep, &buf->lock) == 0) {
        spinlock_unlock(&buf->lock);
    }
    return buf;
}

buf_t *bio_read(buf_t *buf) {
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

    assertk(data != NULL);
    q_memcpy(buf->data, data, SECTOR_SIZE);
    buf->flag |= (BUF_DIRTY | BUF_VALID);
    disk.rw(buf);

    if (block_thread(&buf->sleep, &buf->lock) == 0) {
        spinlock_unlock(&buf->lock);
    }
}


void bio_free(buf_t *buf) {
    assertk(buf->ref_cnt > 0);
    spinlock_lock(&cache.lock);
    if (buf <= &cache.buf[N_BUF] && buf >= &cache.buf[0]) {
        if ((--buf->ref_cnt) == 0 && !queue_empty(&cache.sleep)) {
            //唤醒等待获取 buf 的线程
            unblock_thread(queue_head(&cache.sleep));
        }
    }
    spinlock_unlock(&cache.lock);
}

INT disk_isr(UNUSED interrupt_frame_t *frame) {
    disk.isr(frame);
    pic2_eoi(32 + 14);
}

// ============ test =========
#ifdef TEST

void test_ide_rw() {
    test_start

    disk.rw = ide_rw;
    disk.isr = ide_isr_handler;

    uint8_t data[SECTOR_SIZE];
    q_memset(data, 1, SECTOR_SIZE);
    buf_t *buf = bio_get(1);
    buf_t *buf2 = bio_get(2);
    buf2->no_secs = 1;

    bio_write(buf, data);
    bio_read_sync(buf2);
    assertk(q_memcmp(data, buf2->data, SECTOR_SIZE));

    bio_free(buf);
    bio_free(buf2);
    test_pass
}

void test_dma_rw() {
    test_start
    assertk(dma_dev.dma);
    disk.rw = dma_rw;
    disk.isr = dma_isr_handler;

    uint8_t data[SECTOR_SIZE];
    q_memset(data, 2, SECTOR_SIZE);
    buf_t *buf = bio_get(1);
    buf_t *buf2 = bio_get(2);
    buf2->no_secs = 1;

    bio_write(buf, data);
    bio_read_sync(buf2);
    assertk(q_memcmp(data, buf2->data, SECTOR_SIZE));

    bio_free(buf);
    bio_free(buf2);
    test_pass
}


#endif // TEST