//
// Created by pjs on 2021/4/7.
//
// TODO: 页缓存分块读写而不是以页为单位读写
// TODO: 锁加得太烂了,重新设计数据结构
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
#include "sched/timer.h"


static void create_flush_thread();

static list_head_t *flush;

static struct cache {
    list_head_t list;       // 缓存页面列表
    list_head_t wait;       // 缓存页面列表
    u16_t dirty_cnt;        // 脏页数量,超过一定数量唤醒回写线程
    spinlock_t lock;        // 读写 cache 元素, 修改 list/wait 链表需要加锁
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
    list_header_init(&cache.list);
    list_header_init(&cache.wait);
    cache.dirty_cnt = 0;

    disk.start = ide_start;
    disk.isr = ide_isr;
//    disk.rw = dma_dev.dma ? dma_rw : ide_rw;
//    disk.isr = dma_dev.dma ? dma_isr_handler : ide_isr_handler;

    // 注册 ide 中断
    reg_isr(32 + 14, disk_isr);
    create_flush_thread();
}

static buf_t *new_buf(uint32_t no_secs) {
    buf_t *buf = kmalloc(sizeof(buf_t));
    buf->data = kmalloc(PAGE_SIZE);
    buf->flag = 0;
    buf->no_secs = no_secs;

    list_header_init(&buf->list);
    list_header_init(&buf->sleep);
    return buf;
}


buf_t *page_get(uint32_t no_secs) {
    buf_t *buf;
    list_head_t *head;
    spinlock_lock(&cache.lock);

    list_for_each(head, &cache.list) {
        buf = buf_entry(head);
        // 之后 new_node 修改 node_secs,读取不需要加锁
        if (buf->no_secs == no_secs) {
            return buf;
        }
    }

    list_for_each(head, &cache.wait) {
        buf = buf_entry(head);
        if (buf->no_secs == no_secs) {
            return buf;
        }
    }

    if (!buf) {
        buf = new_buf(no_secs);
        list_add_prev(&buf->list, &cache.list);
    }

    spinlock_unlock(&cache.lock);
    return buf;
}


buf_t *page_read(buf_t *buf) {
    assertk(buf);
    spinlock_lock(&cache.lock);
    if (buf->flag & BUF_VALID) {
        spinlock_unlock(&cache.lock);
        return buf;
    }

    page_rw(buf);
    block_thread(&buf->sleep, &cache.lock);
    spinlock_unlock(&cache.lock);
    return buf;
}

void page_write(buf_t *buf, void *data) {
    assertk(data && buf);
    spinlock_lock(&cache.lock);
    if (buf->flag & BUF_BSY) {
        block_thread(&buf->sleep, &cache.lock);
    }

    q_memcpy(buf->data, data, BUF_SIZE);
    buf->flag |= BUF_DIRTY | BUF_VALID;

    cache.dirty_cnt++;
    if (cache.dirty_cnt > MAX_DIRTY_PAGE) {
        unblock_thread(flush);
    }

    spinlock_unlock(&cache.lock);
}

// 立即刷新内存
void page_write_sync(buf_t *buf, void *data) {
    assertk(buf && data);
    spinlock_lock(&cache.lock);
    if (buf->flag & BUF_BSY) {
        block_thread(&buf->sleep, &cache.lock);
    }

    q_memcpy(buf->data, data, BUF_SIZE);
    buf->flag |= BUF_DIRTY | BUF_VALID;
    cache.dirty_cnt++;
    page_rw(buf);

    block_thread(&buf->sleep, &cache.lock);
    spinlock_unlock(&cache.lock);
}

// 不使用缓冲区数据,强制重新读取磁盘
buf_t *page_read_sync(buf_t *buf) {
    assertk(buf);
    spinlock_lock(&cache.lock);

    if (buf->flag & BUF_BSY) {
        spinlock_unlock(&cache.lock);
        return buf;
    }

    page_rw(buf);
    block_thread(&buf->sleep, &cache.lock);
    spinlock_unlock(&cache.lock);
    return buf;
}

static void page_rw(buf_t *buf) {
    list_del(&buf->list);
    list_add_next(&buf->list, &cache.wait);
    if (cache.list.next == &buf->list) {
        buf->flag |= BUF_BSY;
        disk.start(buf, buf->flag & BUF_DIRTY);
    }
}


INT disk_isr(UNUSED interrupt_frame_t *frame) {
    spinlock_lock(&cache.lock);
    list_head_t *h = cache.wait.next;
    buf_t *buf;
    if (h) {
        buf->flag &= ~(BUF_BSY | BUF_DIRTY);
        list_del(&buf->list);
        list_add_next(&buf->list, &cache.list);

        unblock_threads(&buf->sleep);
        // disk.isr 与 disk.start 可能会耗费大量时间
        // 因此将函数调用放到 cache.lock 锁外
        disk.isr(buf, buf->flag & BUF_DIRTY);
        buf->timestamp = cur_timestamp();

        buf->flag |= BUF_VALID;
        if (buf->flag & BUF_DIRTY) {
            cache.dirty_cnt--;
        }

        if (&cache.wait != h->next) {
            buf = buf_entry(h->next);
            buf->flag |= BUF_BSY;
            disk.start(buf, buf->flag & BUF_DIRTY);
        }
    }
    spinlock_unlock(&cache.lock);
    pic2_eoi(32 + 14);
}

_Noreturn static void *page_flush_worker() {
    while (1) {
        buf_t *buf;
        list_head_t *head;
        spinlock_lock(&cache.lock);
        list_for_each(head, &cache.list) {
            buf = buf_entry(head);
            if (buf->flag & (BUF_DIRTY | BUF_BSY)) {
                page_rw(buf);
            }
            block_thread(&buf->sleep, &cache.lock);
        }
        spinlock_unlock(&cache.lock);
        assertk(ms_sleep(WRITE_BACK_INTERVAL * 1000));
    }
}

static void create_flush_thread() {
    kthread_t tid;
    kt_create(&flush, &tid, page_flush_worker, NULL);
    q_memcpy(tcb_entry(flush)->name, "page_flash", sizeof("page_flash"));
    block_thread(flush, NULL);
}

// size 为需要回收的内存大小(实际回收的内存可能小于size)
void page_recycle(u32_t size) {
    list_head_t *hdr;
    buf_t *buf;
    u64_t cur = cur_timestamp();
    spinlock_lock(&cache.lock);
    // 向前遍历
    hdr = cache.list.prev;
    while (hdr != &cache.list && size > 0) {
        if (size <= 0) return;
        buf = buf_entry(hdr);
        hdr = hdr->prev;
        if (!(buf->flag & (BUF_BSY | BUF_DIRTY)) && cur - buf->timestamp >= CACHE_EXPIRES * 1000) {
            list_del(&buf->list);
            kfree(buf->data);
            kfree(buf);
            size -= PAGE_SIZE;
        }
    }
    spinlock_unlock(&cache.lock);
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