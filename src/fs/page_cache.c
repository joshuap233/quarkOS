//
// Created by pjs on 2021/4/7.
//
//磁盘缓冲块

#include "fs/page_cache.h"
#include "drivers/ide.h"
#include "lib/qstring.h"
#include "lib/list.h"
#include "sched/kthread.h"
#include "lib/qlib.h"
//#include "drivers/dma.h"
#include "mm/kmalloc.h"
#include "drivers/cmos.h"
#include "buf.h"


static struct cache {
    list_head_t dirty_list;  // 脏页列表
    list_head_t active_list; // 已经读取的有效页面
    list_head_t wait_list;   // 等待读取的列表

    u32_t dirty_page_cnt;   // 统计脏页数量,超过一定数量唤醒回写线程
    spinlock_t lock;
    queue_t sleep;           // 睡眠等待缓存块队列
} cache;


static struct disk {
    void (*start)(buf_t *, bool);

    void (*isr)(buf_t *, bool);
} disk;

INT disk_isr(UNUSED interrupt_frame_t *frame);

static void page_rw(buf_t *buf);

void page_cache_init() {
    // 循环队列
    spinlock_init(&cache.lock);
    queue_init(&cache.sleep);
    list_header_init(&cache.dirty_list);
    list_header_init(&cache.active_list);
    list_header_init(&cache.wait_list);
    cache.dirty_page_cnt = 0;

    disk.start = ide_start;
    disk.isr = ide_isr;
//    disk.rw = dma_dev.dma ? dma_rw : ide_rw;
//    disk.isr = dma_dev.dma ? dma_isr_handler : ide_isr_handler;

    // 注册 ide 中断
    reg_isr(32 + 14, disk_isr);
}

static buf_t *new_buf(uint32_t no_secs) {
    buf_t *buf = kmalloc(sizeof(buf_t));
    buf->data = kmalloc(PAGE_SIZE);
    buf->flag = 0;
    buf->no_secs = no_secs;

    spinlock_init(&buf->lock);
    list_header_init(&buf->queue);
    list_header_init(&buf->sleep);
}


static buf_t *page_search(uint32_t no_secs, list_head_t *header) {
    list_head_t *head;
    list_for_each(head, header) {
        buf_t *buf = buf_entry(head);
        if (buf->no_secs == no_secs)
            return buf;
    }
    return NULL;
}

buf_t *page_get(uint32_t no_secs) {
    //用扇区号查找缓冲块, 阻塞直到有可以缓冲块
    spinlock_lock(&cache.lock);
    buf_t *buf;
    buf = page_search(no_secs, &cache.active_list);

    if (!buf) {
        buf = page_search(no_secs, &cache.dirty_list);
    }

    if (!buf) {
        buf = page_search(no_secs, &cache.wait_list);
    }

    if (!buf) {
        buf = new_buf(no_secs);
    }
    spinlock_unlock(&cache.lock);
    return buf;
}


buf_t *page_read(buf_t *buf) {
    assertk(buf);

    spinlock_lock(&buf->lock);
    if (buf->flag & BUF_VALID) {
        spinlock_unlock(&buf->lock);
        return buf;
    }

    page_rw(buf);
    // 可能在 block_thread 调用前,磁盘中断处理程序以及被调用
    // 中断处理程序会释放 buf->lock
    if (block_thread(&buf->sleep, &buf->lock) == 0) {
        spinlock_unlock(&buf->lock);
    }
    return buf;
}

void page_write(buf_t *buf, void *data) {
    assertk(data && buf);

    spinlock_lock(&buf->lock);
    if (buf->flag & BUF_BSY) {
        block_thread(&buf->sleep, &buf->lock);
    }

    q_memcpy(buf->data, data, BUF_SIZE);
    buf->flag |= BUF_DIRTY | BUF_VALID;

    spinlock_lock(&cache.lock);
    list_del(&buf->queue);
    list_add_next(&buf->queue, &cache.dirty_list);
    spinlock_unlock(&cache.lock);

    cache.dirty_page_cnt++;

    spinlock_unlock(&buf->lock);
}

// 立即刷新内存
void page_write_sync(buf_t *buf, void *data) {
    assertk(buf && data);

    spinlock_lock(&buf->lock);
    if (buf->flag & BUF_BSY) {
        block_thread(&buf->sleep, &buf->lock);
    }

    q_memcpy(buf->data, data, BUF_SIZE);
    buf->flag |= BUF_DIRTY | BUF_VALID;
    cache.dirty_page_cnt++;
    page_rw(buf);

    bool locked = block_thread(&buf->sleep, &buf->lock);

    if (locked) {
        spinlock_unlock(&buf->lock);
    }
}

// 不使用缓冲区数据,强制重新读取磁盘
buf_t *page_read_sync(buf_t *buf) {
    assertk(buf);
    spinlock_lock(&buf->lock);

    if (buf->flag & BUF_BSY) {
        spinlock_unlock(&buf->lock);
        return buf;
    }

    page_rw(buf);
    bool locked = block_thread(&buf->sleep, &buf->lock);

    if (locked) {
        spinlock_unlock(&buf->lock);
    }

    return buf;
}

static void page_rw(buf_t *buf) {
    queue_put(&buf->queue, &cache.wait_list);
    if (&buf->queue == &cache.wait_list) {
        buf->flag |= BUF_BSY;
        disk.start(buf, buf->flag & BUF_DIRTY);
    }
}


INT disk_isr(UNUSED interrupt_frame_t *frame) {
    queue_t *h = queue_get(&cache.wait_list);
    if (h) {

        buf_t *buf = buf_entry(h);
        unblock_threads(&buf->sleep);
        disk.isr(buf, buf->flag & BUF_DIRTY);
        buf->timestamp = cur_timestamp();

        buf->flag |= BUF_VALID;
        if (buf->flag & BUF_DIRTY) {
            cache.dirty_page_cnt--;
        }
        buf->flag &= ~(BUF_BSY | BUF_DIRTY);

        list_add_next(&buf->queue, &cache.active_list);

        if (!queue_empty(&cache.wait_list)) {
            buf_t *next = buf_entry(queue_head(&cache.wait_list));
            next->flag |= BUF_BSY;
            disk.start(next, next->flag & BUF_DIRTY);
        }
        // 通知线程, 中断处理程序已经被执行
        spinlock_unlock(&buf->lock);
    }
    pic2_eoi(32 + 14);
}

static void page_flash_worker() {
    list_head_t *head, *next;
    spinlock_lock(&cache.lock);
    list_for_each_del(head, next, &cache.dirty_list) {
        buf_t *buf = buf_entry(head);
        page_rw(buf);

        bool locked = block_thread(&buf->sleep, &cache.lock);

        list_del(&buf->queue);
        list_add_prev(&buf->queue, &cache.active_list);
        if (locked) {
            spinlock_unlock(&cache.lock);
        }
    }
}

static void create_flush_thread() {

}

void page_recycle() {

}

// ============ test =========
#ifdef TEST
// data 放 test_ide_rw 函数里面会导致栈溢出
uint8_t data[BUF_SIZE];

void test_ide_rw() {
    test_start

    disk.start = ide_start;
    disk.isr = ide_isr;

    q_memset(data, 1, BUF_SIZE);
    buf_t *buf = page_get(1);
    buf_t *buf2 = page_get(2);
    buf2->no_secs = 1;

    page_write(buf, data);
    page_read_sync(buf2);
    assertk(q_memcmp(data, buf2->data, BUF_SIZE));

    test_pass
}

//void test_dma_rw() {
//    test_start
////    assertk(dma_dev.dma);
////    disk.rw = dma_rw;
////    disk.isr = dma_isr_handler;
//
//    uint8_t data[SECTOR_SIZE];
//    q_memset(data, 2, SECTOR_SIZE);
//    buf_t *buf = bio_get(1);
//    buf_t *buf2 = bio_get(2);
//    buf2->no_secs = 1;
//
//    bio_write(buf, data);
//    bio_read_sync(buf2);
//    assertk(q_memcmp(data, buf2->data, SECTOR_SIZE));
//
//    bio_free(buf);
//    bio_free(buf2);
//    test_pass
//}


#endif // TEST