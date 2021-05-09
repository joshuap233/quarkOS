//
// Created by pjs on 2021/4/8.
//

#ifndef QUARKOS_BUF_H
#define QUARKOS_BUF_H

#include "types.h"
#include "sched/sleeplock.h"
#include "lib/list.h"
#include "mm/mm.h"

/*
 * BUF_DIRTY: 数据需要更新
 * BUF_VALID: 缓冲块数据为有效数据
 * BUF_BSY:   数据块正在读写
 * BUF_WAIT:  数据块等待读写
 * ref_cnt:   当前块被引用的次数
 * timestamp: 上次访问该页的时间
 * no_secs:   需要读写的扇区起始 lba 值
 * sleep      睡眠线程队列, queue 指向 tcb run_list
 *            等待同步到磁盘的队列,DIRTY位被设置时则需要写回磁盘,
 *            否则需要从磁盘读取,操作完成后从队列删除,并清空 dirty 位
 */
typedef struct page_cache {
#define BUF_DIRTY   1
#define BUF_VALID   (1<<1)
#define BUF_BSY     (1<<2)
#define SECTOR_SIZE 512
#define BUF_SIZE    PAGE_SIZE
    u32_t timestamp;

    uint8_t *data;

    uint16_t flag;
//    uint16_t ref_cnt;
    uint32_t no_secs; // 页对应的起始扇区lab值

    list_head_t list;    // 等待读写队列
    queue_t sleep;       // 睡眠线程队列
} buf_t;

#define buf_entry(ptr) list_entry(ptr,buf_t,list)

#endif //QUARKOS_BUF_H
