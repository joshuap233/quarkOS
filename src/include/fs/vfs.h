//
// Created by pjs on 2021/5/13.
//

#ifndef QUARKOS_FS_VFS_H
#define QUARKOS_FS_VFS_H

#include "types.h"

#define FILE_NAME_LEN 128

// 文件描述符
// TCB 添加 fd_t pwd
typedef struct file_descriptor {
    char name[FILE_NAME_LEN];
    u16_t mode;
    u32_t bid;       // 结构对应的数据所在的块 id
    u32_t flags;
    u32_t inode_num; // inode 号
    u32_t size;      // 文件大小
    u32_t fs_type;

    u32_t create_time;
    u32_t access_time;
    u32_t modified_time;

    u32_t offset;    // 用于常规文件读写
} fd_t;


#endif //QUARKOS_FS_VFS_H
