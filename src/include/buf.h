//
// Created by pjs on 2021/4/8.
//

#ifndef QUARKOS_BUF_H
#define QUARKOS_BUF_H

#include "types.h"
#include "sched/sleeplock.h"
#include "lib/list.h"

typedef struct buf {
#define BUF_DIRTY 1     //数据需要更新
#define BUF_VALID 1<<1  //数据块有效
#define BUF_BSY   1<<2  //数据块正在读写
#define SECTOR_SIZE 512
#define N_BUF       30

    uint8_t flag;
    uint16_t ref_cnt;      // 当前块被引用的次数
    spinlock_t lock;
    uint32_t no_secs;      //需要读写的扇区起始 lba 值
    uint8_t data[SECTOR_SIZE];
    queue_t queue;
    // 睡眠线程队列, queue 指向 tcb run_list
    // 等待同步到磁盘的队列,DIRTY位被设置时则需要写回磁盘,否则需要从磁盘读取
    // 操作完成后从队列删除,并清空 dirty 位
} buf_t;

#define buf_entry(ptr) list_entry(ptr,buf_t,queue)

#endif //QUARKOS_BUF_H
