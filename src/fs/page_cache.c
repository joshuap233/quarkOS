//
// Created by pjs on 2021/4/7.
//
// TODO: 页缓存分块读写(仅写入脏块)
// 有个死锁 BUG 没找到
#include "fs/page_cache.h"
#include "drivers/ide.h"
#include "lib/qstring.h"
#include "lib/list.h"
#include "sched/kthread.h"
#include "lib/qlib.h"
#include "mm/kmalloc.h"
#include "drivers/cmos.h"
#include "buf.h"
#include "sched/timer.h"
#include "lib/rwlock.h"
#include "sched/sleeplock.h"


static void create_flush_thread();

list_head_t *flush_worker;

static struct cache {
    struct head {
        list_head_t head;
        rwlock_t rwlock;    // head 链表操作需要加锁(链表增删改查)
    } list;                 // 有效缓存页列表

    struct sleep {          //正在读写的线程
        list_head_t *thread;
        spinlock_t signal;
    } sleep;

    buf_t *page_writing;     // 正在被磁盘读写的页
    sleeplock_t wait_rw;     // 缓冲区读写磁盘时加锁
} cache;

static struct disk {
    void (*start)(buf_t *, bool);

    void (*isr)(buf_t *, bool);
} disk;

INT disk_isr(UNUSED interrupt_frame_t *frame);

static void page_rw(buf_t *buf);

static void page_flush(buf_t *buf);

INLINE void _sleeplock_lock(rwlock_t *lk);

INLINE void _sleeplock_unlock(rwlock_t *lk);

static void sleep();

static void wakeup();

void page_cache_init() {
    sleeplock_init(&cache.wait_rw);
    list_header_init(&cache.list.head);
    rwlock_init(&cache.list.rwlock);

    spinlock_init(&cache.sleep.signal);
    cache.sleep.thread = NULL;

    cache.page_writing = NULL;
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
    buf->timestamp = 0;
    rwlock_init(&buf->rwlock);
    list_header_init(&buf->list);
    return buf;
}


buf_t *page_get(uint32_t no_secs) {
    // no_secs 必须为 8 的整数倍
    assertk(!(no_secs & 7));
    buf_t *buf;
    list_head_t *head;

    rwlock_rLock(&cache.list.rwlock);
    list_for_each(head, &cache.list.head) {
        buf = buf_entry(head);
        if (buf->no_secs == no_secs) {
            rwlock_rUnlock(&cache.list.rwlock);
            return buf;
        }
    }
    rwlock_rUnlock(&cache.list.rwlock);

    buf = new_buf(no_secs);
    rwlock_wLock(&cache.list.rwlock);
    list_add_prev(&buf->list, &cache.list.head);
    rwlock_wUnlock(&cache.list.rwlock);

    return buf;
}

buf_t *page_read_no(uint32_t no_secs) {
    buf_t *buf = page_get(no_secs);
    page_read(buf);
    return buf;
}

buf_t *page_read_no_sync(uint32_t no_secs) {
    buf_t *buf = page_get(no_secs);
    page_read_sync(buf);
    return buf;
}

void page_read(buf_t *buf) {
    assertk(buf);
    rwlock_rLock(&buf->rwlock);
    if (buf->flag & BUF_VALID) {
        rwlock_rUnlock(&buf->rwlock);
        return;
    }
    rwlock_rUnlock(&buf->rwlock);

    // disk_isr 会修改 buf, 因此需要用写锁
    _sleeplock_lock(&buf->rwlock);
    page_rw(buf);
    _sleeplock_unlock(&buf->rwlock);
}


void page_write(buf_t *buf) {
    assertk(buf);
    rwlock_wLock(&buf->rwlock);

    buf->flag |= BUF_DIRTY | BUF_VALID;

    rwlock_wUnlock(&buf->rwlock);
}

static void page_rw(buf_t *buf) {
    cache.page_writing = buf;
    cache.sleep.thread = &CUR_HEAD;
    disk.start(buf, buf->flag & BUF_DIRTY);
    sleep();
}


// 立即刷新内存
void page_write_sync(buf_t *buf) {
    assertk(buf);
    _sleeplock_lock(&buf->rwlock);

    buf->flag |= BUF_DIRTY | BUF_VALID;
    page_rw(buf);

    _sleeplock_unlock(&buf->rwlock);
}

// 不使用缓冲区数据,强制重新读取磁盘
void page_read_sync(buf_t *buf) {
    assertk(buf);
    _sleeplock_lock(&buf->rwlock);

    page_rw(buf);

    _sleeplock_unlock(&buf->rwlock);
}


INT disk_isr(UNUSED interrupt_frame_t *frame) {
    buf_t *buf = cache.page_writing;
    if (buf) {
        disk.isr(buf, buf->flag & BUF_DIRTY);

        buf->flag &= ~BUF_DIRTY;
        buf->flag |= BUF_VALID;

        cache.page_writing = NULL;
        buf->timestamp = cur_timestamp();

        rwlock_wLock(&cache.list.rwlock);
        list_del(&buf->list);
        list_add_next(&buf->list, &cache.list.head);
        rwlock_wUnlock(&cache.list.rwlock);

        wakeup();
    }
    pic2_eoi(32 + 14);
}

static void page_flush(buf_t *buf) {
    rwlock_rLock(&buf->rwlock);
    assertk(buf->flag & BUF_DIRTY);
    rwlock_rUnlock(&buf->rwlock);

    _sleeplock_lock(&buf->rwlock);

    page_rw(buf);

    _sleeplock_unlock(&buf->rwlock);
}

_Noreturn static void *page_flush_worker() {
    buf_t *buf;
    list_head_t *head;
    while (1) {
        // TODO: 脏页排序再写入
        rwlock_rLock(&cache.list.rwlock);
        list_for_each(head, &cache.list.head) {
            buf = buf_entry(head);
            if (buf->flag & BUF_DIRTY) {
                // 必须先释放读锁,否则 irs 中的写锁死锁
                rwlock_rUnlock(&cache.list.rwlock);
                page_flush(buf);
                rwlock_rLock(&cache.list.rwlock);
            }
        }
        rwlock_rUnlock(&cache.list.rwlock);
        assertk(ms_sleep(WRITE_BACK_INTERVAL * 1000));
    }
}

static void create_flush_thread() {
    kthread_t tid;
    kthread_create(&tid, page_flush_worker, NULL);
    kthread_set_name(tid, "page_flash");
    flush_worker = kthread_get_run_list(tid);
}

static void recycle(buf_t *buf) {
    list_del(&buf->list);
    kfree(buf->data);
    kfree(buf);
}

// size 为需要回收的内存大小(实际回收的内存可能小于size)
void page_recycle(u32_t size) {
    list_head_t *hdr;
    buf_t *buf;
    u64_t cur = cur_timestamp();

    // 向前遍历
    rwlock_wLock(&cache.list.rwlock);
    hdr = cache.list.head.prev;
    while (hdr != &cache.list.head && size > 0) {
        if (size <= 0) return;
        buf = buf_entry(hdr);
        hdr = hdr->prev;
        if (!(buf->flag & BUF_DIRTY) && cur - buf->timestamp >= CACHE_EXPIRES * 1000) {
            recycle(buf);
            size -= PAGE_SIZE;
        }
    }
    rwlock_wUnlock(&cache.list.rwlock);
}

// 使用下面的函数对 sleeplock 进行加锁,防止加速顺序不同导致死锁
INLINE void _sleeplock_lock(rwlock_t *lk) {
    sleeplock_lock(&cache.wait_rw);
    rwlock_wLock(lk);
}

INLINE void _sleeplock_unlock(rwlock_t *lk) {
    rwlock_wUnlock(lk);
    sleeplock_unlock(&cache.wait_rw);
}

static void sleep() {
    spinlock_lock(&cache.sleep.signal);
    // cache.sleep.thread == NULL,则 wakeup 已经运行
    if (cache.sleep.thread) {
        block_thread(NULL, &cache.sleep.signal);
    }
    spinlock_unlock(&cache.sleep.signal);
}

// wakeup 在中断处理程序中调用,在中断程序中使用自旋锁可能会造成死锁
// wakeup 使用 trylock 而不是 spinlock_lock,
// 如果过获取到锁,则 sleep 调用线程运行到 spinlock_lock 前,
// 或者线程已经睡眠,因此尝试唤醒线程
// 如果没有获取到锁那么 sleep 调用线程运行到 lock 与 block_thread 之间,
// 如果 block_thread 检测到传入的锁没有被上锁,则不会睡眠
static void wakeup() {
    assertk(cache.sleep.thread);
    if (spinlock_trylock(&cache.sleep.signal)) {
        unblock_thread(cache.sleep.thread);
        cache.sleep.thread = NULL;
    }
    spinlock_unlock(&cache.sleep.signal);
}

// ============ test =========
#ifdef TEST
// data 放 test_ide_rw 函数里面会导致栈(4096 byte)溢出
uint8_t tmp[BUF_SIZE];

static u8_t list_cnt() {
    u8_t cnt = 0;
    for (list_head_t *hdr = cache.list.head.next; hdr != &cache.list.head; hdr = hdr->next) {
        cnt++;
    }
    return cnt;
}

#define  assert_cmp(buf, value)  {\
    for (int i = 0; i < BUF_SIZE; ++i) { \
        assertk(((char*)(buf)->data)[i] == (value));\
    }\
}

void test_ide_rw() {
    test_start
    disk.start = ide_start;
    disk.isr = ide_isr;

    buf_t *buf0 = page_get(0);
    buf_t *buf1 = page_get(8);
    buf1->no_secs = 0;

    // 保存初始值
    page_read_sync(buf0);
    q_memcpy(tmp, buf0->data, BUF_SIZE);

    q_memset(buf0->data, 1, BUF_SIZE);
    page_write(buf0);
    page_flush(buf0);
    page_read_sync(buf1);
//    assert_cmp(buf1, 1);
    for (int i = 0; i < BUF_SIZE; ++i) {
        assertk(((char *) buf0->data)[i] == 1);
    }

    q_memset(buf0->data, 2, BUF_SIZE);
    page_write_sync(buf0);
    page_read_sync(buf1);
    assert_cmp(buf1, 2);

    //恢复初始值
    q_memcpy(buf0->data, tmp, BUF_SIZE);
    page_write_sync(buf0);
    page_read_sync(buf1);
    assertk(q_memcmp(tmp, buf1->data, BUF_SIZE));

    //测试回收
    recycle(buf0);
    recycle(buf1);
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