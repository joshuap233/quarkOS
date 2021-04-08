//
// Created by pjs on 2021/4/8.
//

#ifndef QUARKOS_BUF_H
#define QUARKOS_BUF_H
#include "param.h"
#include "types.h"

typedef struct buf {
#define BUF_DIRTY 1     //数据需要更新
#define BUF_VALID 1<<1  //数据块有效
#define BUF_BSY   1<<2  //数据块正在读写
// TODO: 加读写锁?
    uint8_t flag;
    uint16_t ref_cnt;      // 当前块被引用的次数
    uint32_t no_secs;      //需要读写的扇区起始 lba 值
    uint8_t data[SECTOR_SIZE];
    struct buf *next;      //缓冲链表
    struct buf *q_next;
    // 等待读写的队列,DIRTY位被设置时则需要写回磁盘,否则需要从磁盘读取
    // 操作完成后从队列删除,并清空 dirty 位
    // 由 ide 驱动管理
}    buf_t;

#endif //QUARKOS_BUF_H
