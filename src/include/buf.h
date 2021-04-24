//
// Created by pjs on 2021/4/8.
//

#ifndef QUARKOS_BUF_H
#define QUARKOS_BUF_H

#include "types.h"
#include "sched/sleeplock.h"
#include "lib/list.h"

/*
 * BUF_DIRTY: 数据需要更新
 * BUF_VALID: 缓冲块数据为有效数据
 * BUF_BSY:   数据块正在读写
 * ref_cnt:   当前块被引用的次数
 * no_secs:   需要读写的扇区起始 lba 值
 * queue      睡眠线程队列, queue 指向 tcb run_list
 *            等待同步到磁盘的队列,DIRTY位被设置时则需要写回磁盘,
 *            否则需要从磁盘读取,操作完成后从队列删除,并清空 dirty 位
 */
typedef struct buf {
#define BUF_DIRTY   1
#define BUF_VALID   (1<<1)
#define BUF_BSY     (1<<2)
#define SECTOR_SIZE 512
#define BUF_SIZE    SECTOR_SIZE
#define N_BUF       30
    uint8_t flag;
    uint16_t ref_cnt;
    spinlock_t lock;
    uint32_t no_secs;
    uint8_t *data;
    queue_t queue; // 等待读写队列
    queue_t sleep; // 睡眠队列
} buf_t;

#define buf_entry(ptr) list_entry(ptr,buf_t,queue)

#endif //QUARKOS_BUF_H
