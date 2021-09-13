//
// Created by pjs on 2021/4/7.
//
// TODO: 页缓存分块读写(仅写入脏块)
#include <fs/writeback.h>
#include <drivers/ide.h>
#include <lib/qstring.h>
#include <lib/list.h>
#include <task/fork.h>
#include <lib/qlib.h>
#include <mm/kmalloc.h>
#include <drivers/cmos.h>
#include <task/timer.h>
#include <lib/rwlock.h>
#include <task/thread_mutex.h>
#include <lib/irlock.h>
#include <mm/page.h>
#include <mm/page_alloc.h>
#include <mm/kvm.h>
#include <fs/vfs.h>

static void create_flush_thread();

static list_head_t *flush_worker;

static struct cache {
    struct head {
        list_head_t head;
        rwlock_t rwlock;    // head 链表操作需要加锁(链表增删改查)
    } list;                 // 有效缓存页列表

    list_head_t *thread;

    struct page *page_writing;  // 正在被磁盘读写的页
    thread_mutex_t wait_rw;     // 缓冲区读写磁盘时加锁

    lf_queue dirty;         // 脏页
    lf_queue inode_dirty;   // 脏 inode
} cacheAllocator;

static struct disk {
    void (*start)(struct page *, bool);

    void (*isr)(struct page *, bool);
} disk;

INT disk_isr(UNUSED interrupt_frame_t *frame);

static void page_rw(struct page *buf);

static void page_flush(struct page *buf);

INLINE void sleeplock_lock(rwlock_t *lk);

INLINE void sleeplock_unlock(rwlock_t *lk);

void page_cache_init() {

    thread_mutex_init(&cacheAllocator.wait_rw);
    list_header_init(&cacheAllocator.list.head);
    rwlock_init(&cacheAllocator.list.rwlock);

    lfQueue_init(&cacheAllocator.dirty);
    lfQueue_init(&cacheAllocator.inode_dirty);
    cacheAllocator.thread = NULL;
    cacheAllocator.page_writing = NULL;
    disk.start = ide_start;
    disk.isr = ide_isr;
//    disk.rw = dma_dev.dma ? dma_rw : ide_rw;
//    disk.isr = dma_dev.dma ? dma_isr_handler : ide_isr_handler;

    // 注册 ide 中断
    reg_isr(IRQ0 + 14, disk_isr);
    create_flush_thread();
}

static struct page *new_buf(uint32_t no_secs) {
    struct page *buf = __alloc_page(PAGE_SIZE);
    struct pageCache *cache = &buf->pageCache;

    kvm_map(buf, VM_PRES | VM_KW);
    buf->flag |= PG_CACHE;
    cache->no_secs = no_secs;
    cache->timestamp = 0;
    lfQueue_node_init(&cache->dirty);
    return buf;
}


struct page *page_get(uint32_t no_secs) {
    // no_secs 必须为 8 的整数倍
    assertk(!(no_secs & 7));
    struct page *buf;
    list_head_t *head;
    struct pageCache *cache;

    rwlock_rLock(&cacheAllocator.list.rwlock);
    list_for_each(head, &cacheAllocator.list.head) {
        buf = PAGE_ENTRY(head);
        cache = &buf->pageCache;
        if (cache->no_secs == no_secs) {
            assertk(buf->flag & PG_CACHE);
            rwlock_rUnlock(&cacheAllocator.list.rwlock);
            return buf;
        }
    }
    rwlock_rUnlock(&cacheAllocator.list.rwlock);

    buf = new_buf(no_secs);
    rwlock_wLock(&cacheAllocator.list.rwlock);
    list_add_prev(&buf->head, &cacheAllocator.list.head);
    rwlock_wUnlock(&cacheAllocator.list.rwlock);

    return buf;
}

struct page *page_read_no(uint32_t no_secs) {
    struct page *buf = page_get(no_secs);
    page_read(buf);
    return buf;
}

struct page *page_read_no_sync(uint32_t no_secs) {
    struct page *buf = page_get(no_secs);
    page_read_sync(buf);
    return buf;
}

void page_read(struct page *buf) {
    assertk(buf);
    rwlock_rLock(&buf->rwlock);
    if (buf->flag & PG_VALID) {
        rwlock_rUnlock(&buf->rwlock);
        return;
    }
    rwlock_rUnlock(&buf->rwlock);

    // disk_isr 会修改 buf, 因此需要用写锁
    sleeplock_lock(&buf->rwlock);
    page_rw(buf);
    sleeplock_unlock(&buf->rwlock);
}


void mark_page_dirty(struct page *buf) {
    assertk(buf);
    rwlock_wLock(&buf->rwlock);

    assertk(buf->flag & PG_CACHE);
    if (!(buf->flag & PG_DIRTY)) {
        buf->flag |= PG_DIRTY | PG_VALID;
        lfQueue_put(&cacheAllocator.dirty, &buf->pageCache.dirty);
    }
    rwlock_wUnlock(&buf->rwlock);
}


// 立即刷新内存
void page_sync(struct page *buf) {
    assertk(buf);
    sleeplock_lock(&buf->rwlock);

    buf->flag |= PG_DIRTY | PG_VALID;
    page_rw(buf);

    sleeplock_unlock(&buf->rwlock);
}

// 不使用缓冲区数据,强制重新读取磁盘
void page_read_sync(struct page *buf) {
    assertk(buf);
    sleeplock_lock(&buf->rwlock);

    page_rw(buf);

    sleeplock_unlock(&buf->rwlock);
}

static void page_rw(struct page *buf) {
    cacheAllocator.page_writing = buf;
    cacheAllocator.thread = &CUR_HEAD;
    disk.start(buf, buf->flag & PG_DIRTY);

    ir_lock_t irLock;
    ir_lock(&irLock);
    // 如果 thread 为 NULL 则 isr 在 ir_lock 前已经执行完成
    if (cacheAllocator.thread)
        task_sleep(NULL, NULL);
    ir_unlock(&irLock);

    rwlock_wLock(&cacheAllocator.list.rwlock);
    list_del(&buf->head);
    list_add_next(&buf->head, &cacheAllocator.list.head);
    rwlock_wUnlock(&cacheAllocator.list.rwlock);
}


INT disk_isr(UNUSED interrupt_frame_t *frame) {
    struct page *buf = cacheAllocator.page_writing;
    if (buf) {
        struct pageCache *cache = &buf->pageCache;

        disk.isr(buf, buf->flag & PG_DIRTY);

        buf->flag &= ~PG_DIRTY;
        buf->flag |= PG_VALID;

        cache->timestamp = cur_timestamp();
        cacheAllocator.page_writing = NULL;

        assertk(cacheAllocator.thread);
        task_wakeup(cacheAllocator.thread);
        cacheAllocator.thread = NULL;
    }
    pic2_eoi(32 + 14);
}

static void page_flush(struct page *buf) {
    rwlock_rLock(&buf->rwlock);
    assertk(buf->flag & PG_DIRTY);
    rwlock_rUnlock(&buf->rwlock);

    sleeplock_lock(&buf->rwlock);

    page_rw(buf);

    sleeplock_unlock(&buf->rwlock);
}

static void flush_pages() {
    // TODO: 脏页排序再写入
    struct page *buf;
    lfq_node *node;

    while ((node = lfQueue_get(&cacheAllocator.dirty))) {
        buf = page_dirty_entry(node);
        page_flush(buf);
    }
}

static void flush_inode() {
    lfq_node *node;
    inode_t *inode;
    node = lfQueue_get(&cacheAllocator.inode_dirty);
    if (node) {
        inode = inode_dirty_entry(node);
        inode->ops->write_super_block(inode->sb);
        inode->ops->write_back(inode);
    }
    while ((node = lfQueue_get(&cacheAllocator.inode_dirty))) {
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
    task_set_name(tid, "page_flash");
    flush_worker = task_get_run_list(tid);
}

static void recycle(struct page *buf) {
    list_del(&buf->head);
    kfree(buf->data);
    kfree(buf);
}

// size 为需要回收的内存大小(实际回收的内存可能小于size)
void page_recycle(u32_t size) {
    list_head_t *hdr;
    struct page *buf;
    u64_t cur = cur_timestamp();

    // 向前遍历
    rwlock_wLock(&cacheAllocator.list.rwlock);
    hdr = cacheAllocator.list.head.prev;
    while (hdr != &cacheAllocator.list.head && size > 0) {
        if (size <= 0) return;
        buf = PAGE_ENTRY(hdr);
        hdr = hdr->prev;
        if (!(buf->flag & PG_DIRTY) &&
            cur - buf->pageCache.timestamp >= CACHE_EXPIRES * 1000) {
            recycle(buf);
            size -= PAGE_SIZE;
        }
    }
    rwlock_wUnlock(&cacheAllocator.list.rwlock);
}

INLINE void sleeplock_lock(rwlock_t *lk) {
    thread_mutex_lock(&cacheAllocator.wait_rw);
    rwlock_wLock(lk);
}

INLINE void sleeplock_unlock(rwlock_t *lk) {
    rwlock_wUnlock(lk);
    thread_mutex_unlock(&cacheAllocator.wait_rw);
}

void mark_inode_dirty(inode_t *inode, enum inode_state state) {
    switch (inode->state) {
        case I_OLD:
            inode->state = state;
            lfQueue_put(&cacheAllocator.inode_dirty, &inode->dirty);
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
        default: {
            assertk(state = I_NEW);
            // 刚创建的新 inode
            inode->state = state;
            lfQueue_put(&cacheAllocator.inode_dirty, &inode->dirty);
        }

    }
}

// ============ test =========
#ifdef TEST
// data 放 test_ide_rw 函数里面会导致栈(4096 byte)溢出
uint8_t tmp[BUF_SIZE];

//UNUSED static u8_t list_cnt() {
//    u8_t cnt = 0;
//    for (list_head_t *hdr = cache.list.head.next; hdr != &cache.list.head; hdr = hdr->next) {
//        cnt++;
//    }
//    return cnt;
//}

#define  assert_cmp(buf, value)  {\
    for (int i = 0; i < BUF_SIZE; ++i) { \
        assertk(((char*)(buf)->data)[i] == (value));\
    }\
}

UNUSED void test_ide_rw() {
    test_start
    disk.start = ide_start;
    disk.isr = ide_isr;

    struct page *buf0 = page_get(0);
    struct page *buf1 = page_get(8);
    buf1->pageCache.no_secs = 0;

    // 保存初始值
    page_read_sync(buf0);
    memcpy(tmp, buf0->data, BUF_SIZE);

    memset(buf0->data, 1, BUF_SIZE);
    mark_page_dirty(buf0);
    page_flush(buf0);
    page_read_sync(buf1);
//    assert_cmp(buf1, 1);
    for (int i = 0; i < BUF_SIZE; ++i) {
        assertk(((char *) buf0->data)[i] == 1);
    }

    memset(buf0->data, 2, BUF_SIZE);
    page_sync(buf0);
    page_read_sync(buf1);
    assert_cmp(buf1, 2);

    //恢复初始值
    memcpy(buf0->data, tmp, BUF_SIZE);
    page_sync(buf0);
    page_read_sync(buf1);
    assertk(memcmp(tmp, buf1->data, BUF_SIZE));

    //测试回收
    recycle(buf0);
    recycle(buf1);
    test_pass
}


#endif // TEST