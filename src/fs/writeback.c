//
// Created by pjs on 2021/4/7.
//
// TODO: 页缓存分块读写(仅写入脏块)
#include <fs/writeback.h>
#include <drivers/ide.h>
#include <lib/qstring.h>
#include <lib/list.h>
#include <sched/kthread.h>
#include <lib/qlib.h>
#include <mm/kmalloc.h>
#include <drivers/cmos.h>
#include <fs/buf.h>
#include <sched/timer.h>
#include <lib/rwlock.h>
#include <sched/thread_mutex.h>
#include <lib/irlock.h>

static void create_flush_thread();

static list_head_t *flush_worker;

static struct cache {
    struct head {
        list_head_t head;
        rwlock_t rwlock;    // head 链表操作需要加锁(链表增删改查)
    } list;                 // 有效缓存页列表

    list_head_t *thread;

    buf_t *page_writing;     // 正在被磁盘读写的页
    thread_mutex_t wait_rw;     // 缓冲区读写磁盘时加锁

    lf_queue dirty;         // 脏页
    lf_queue inode_dirty;   // 脏 inode
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

void page_cache_init() {

    thread_mutex_init(&cache.wait_rw);
    list_header_init(&cache.list.head);
    rwlock_init(&cache.list.rwlock);

    lfQueue_init(&cache.dirty);
    lfQueue_init(&cache.inode_dirty);
    cache.thread = NULL;
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
    buf->ref_cnt = 0;
    rwlock_init(&buf->rwlock);
    list_header_init(&buf->list);
    lfQueue_node_init(&buf->dirty);
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


void mark_page_dirty(buf_t *buf) {
    assertk(buf);
    rwlock_wLock(&buf->rwlock);

    if (!(buf->flag & BUF_DIRTY)){
        buf->flag |= BUF_DIRTY | BUF_VALID;
        lfQueue_put(&cache.dirty, &buf->dirty);
    }
    rwlock_wUnlock(&buf->rwlock);
}


// 立即刷新内存
void page_sync(buf_t *buf) {
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

static void page_rw(buf_t *buf) {
    cache.page_writing = buf;
    cache.thread = &CUR_HEAD;
    disk.start(buf, buf->flag & BUF_DIRTY);

    ir_lock_t irLock;
    ir_lock(&irLock);
    // 如果 thread 为 NULL 则 isr 在 ir_lock 前已经执行完成
    if (cache.thread)
        block_thread(NULL, NULL);
    ir_unlock(&irLock);

    rwlock_wLock(&cache.list.rwlock);
    list_del(&buf->list);
    list_add_next(&buf->list, &cache.list.head);
    rwlock_wUnlock(&cache.list.rwlock);
}


INT disk_isr(UNUSED interrupt_frame_t *frame) {
    buf_t *buf = cache.page_writing;
    if (buf) {
        disk.isr(buf, buf->flag & BUF_DIRTY);

        buf->flag &= ~BUF_DIRTY;
        buf->flag |= BUF_VALID;

        buf->timestamp = cur_timestamp();
        cache.page_writing = NULL;

        assertk(cache.thread);
        unblock_thread(cache.thread);
        cache.thread = NULL;
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

static void flush_pages() {
    // TODO: 脏页排序再写入
    buf_t *buf;
    lfq_node *node;

    while ((node = lfQueue_get(&cache.dirty))) {
        buf = buf_dirty_entry(node);
        page_flush(buf);
    }
}

static void flush_inode() {
    lfq_node *node;
    inode_t *inode;
    node = lfQueue_get(&cache.inode_dirty);
    if (node) {
        inode = inode_dirty_entry(node);
        inode->ops->write_super_block(inode->sb);
        inode->ops->write_back(inode);
    }
    while ((node = lfQueue_get(&cache.inode_dirty))) {
        inode = inode_dirty_entry(node);
        inode->ops->write_back(inode);
    }
}

void page_fsync() {
    flush_inode();
    flush_pages();
}

_Noreturn static void *page_flush_worker() {

    while (1) {
        page_fsync();
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
    thread_mutex_lock(&cache.wait_rw);
    rwlock_wLock(lk);
}

INLINE void _sleeplock_unlock(rwlock_t *lk) {
    rwlock_wUnlock(lk);
    thread_mutex_unlock(&cache.wait_rw);
}

void mark_inode_dirty(inode_t *inode, enum inode_state state) {
    switch (inode->state) {
        case I_OLD:
            inode->state = state;
            lfQueue_put(&cache.inode_dirty, &inode->dirty);
            break;
        case I_NEW:
            if (state == I_DEL)
                inode->state = I_DEL;
            break;
        case I_DATA:
            if (state == I_TIME)
                inode->state = I_NEW;
            else if (state == I_DEL)
                inode->state = I_DEL;
            break;
        case I_TIME:
            if (state == I_DATA)
                inode->state = I_NEW;
            else if (state == I_DEL)
                inode->state = I_DEL;
            break;
        case I_DEL:
            break;
        default:
            // 刚创建的新 inode
            inode->state = state;
            lfQueue_put(&cache.inode_dirty, &inode->dirty);
    }
}

// ============ test =========
#ifdef TEST
// data 放 test_ide_rw 函数里面会导致栈(4096 byte)溢出
uint8_t tmp[BUF_SIZE];

UNUSED static u8_t list_cnt() {
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

UNUSED void test_ide_rw() {
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
    mark_page_dirty(buf0);
    page_flush(buf0);
    page_read_sync(buf1);
//    assert_cmp(buf1, 1);
    for (int i = 0; i < BUF_SIZE; ++i) {
        assertk(((char *) buf0->data)[i] == 1);
    }

    q_memset(buf0->data, 2, BUF_SIZE);
    page_sync(buf0);
    page_read_sync(buf1);
    assert_cmp(buf1, 2);

    //恢复初始值
    q_memcpy(buf0->data, tmp, BUF_SIZE);
    page_sync(buf0);
    page_read_sync(buf1);
    assertk(q_memcmp(tmp, buf1->data, BUF_SIZE));

    //测试回收
    recycle(buf0);
    recycle(buf1);
    test_pass
}


#endif // TEST