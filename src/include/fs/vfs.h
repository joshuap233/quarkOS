//
// Created by pjs on 2021/5/13.
//

#ifndef QUARKOS_FS_VFS_H
#define QUARKOS_FS_VFS_H

#include "types.h"
#include "lib/list.h"
#include "lib/rwlock.h"
#include "buf.h"

typedef struct super_block super_block_t;
typedef struct inode inode_t;
typedef struct directory directory_t;

#define VFS_TP_DIR  0
#define VFS_TP_REG  1

#define FILE_NAME_LEN 128

struct pages {
    buf_t *buf;
    struct pages *next;
};

struct inode {
    u16_t refCnt;
    u16_t mode;

    u16_t linkCnt;
    u16_t type;

    u32_t inode;
    u32_t size;        // 文件大小
    u32_t blockCnt;

    u32_t createTime;
    u32_t accessTime;
    u32_t modifiedTime;
    u32_t offset;      // 用于常规文件读写

    struct directory_t *dir;
    struct super_block *sb;
    struct pages *pages;
    struct inode *parent;
    struct inode *child, *right;
    rwlock_t rwlock;
};

struct directory {
    bool dirty;
    inode_t *inode;
    char name[FILE_NAME_LEN];
    u8_t nameLen;
    rwlock_t rwlock;
};

struct super_block {
    inode_t *root;
    u32_t magic;

    u32_t createTime;
    u32_t accessTime;
    u32_t modifiedTime;
    u32_t mountTime;

    u32_t freeBlockCnt;
    u32_t freeInodeCnt;
    u32_t blockSize;

    rwlock_t rwlock;
};


#endif //QUARKOS_FS_VFS_H
