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

#define FILE_NAME_LEN 128

enum SEEK_WHENCE {
    SEEK_SET = 0, SEEK_CUR, SEEK_END
};


// 所有文件系统必须支持的操作
struct fs_ops {
    inode_t *(*find)(inode_t *parent, const char *name);

    void (*open)(inode_t *inode);

    void (*close)(inode_t *inode);

    u32_t (*read)(inode_t *file, uint32_t offset, uint32_t size, char *buf);

    u32_t (*write)(inode_t *file, uint32_t offset, uint32_t size, char *buf);

    void (*mkdir)(inode_t *parent, const char *name);

    void (*mkfile)(inode_t *parent, const char *name);

    void (*rmdir)(inode_t *dir);

    void (*link)(inode_t *src, inode_t *parent, const char *name);

    void (*unlink)(inode_t *file);

    void (*ls)(inode_t *dir);
};


// vfs 支持的操作
struct vfs_ops {
    inode_t *(*find)(const char *path);

    void (*open)(const char *path);

    void (*close)(inode_t *node);

    u32_t (*read)(inode_t *node, uint32_t size, char *buf);

    u32_t (*write)(inode_t *node, uint32_t size, char *buf);

    void (*lseek)(inode_t *node, u32_t offset, enum SEEK_WHENCE whence);

    void (*mkdir)(const char *path);

    void (*rmdir)(const char *path);

    void (*mkfile)(const char *path);

    void (*unlink)(const char *path);

    void (*link)(const char *src, const char *desc);

    void (*ls)(const char *path);

    void (*umount)();

    void (*mount)(char *path, inode_t *inode);
};

extern struct vfs_ops fs_ops;

// vfs 没有统一的 mode 与 type,
// 因此 mode 与 type 由具体的文件系统管理
struct inode {

    u16_t refCnt;
    u16_t mode;

    u16_t type;
    u16_t linkCnt;

    u32_t size;            // 文件大小
    u32_t pageCnt;         // 数据块数量,部分被映射的数据块存储在 page 链表

    u32_t createTime;
    u32_t accessTime;
    u32_t modifiedTime;
    u32_t deleteTime;

    u32_t offset;         // 用于常规文件读写

    union {
        list_head_t lru;
        list_head_t dirty;
    } list;

    list_head_t page;     // 缓存数据链表

    struct fs_ops *ops;
    struct directory *dir;
    struct super_block *sb;
    rwlock_t rwlock;
};


struct directory {
    bool dirty;
    u32_t ino;
    inode_t *inode;
    char name[FILE_NAME_LEN];
    u8_t nameLen;

    struct directory *parent, *child;
    list_head_t list; // 同层目录
    list_head_t link; // 引用相同 inode 的目录链表
    rwlock_t rwLock;
};

#define dir_entry(ptr) list_entry(ptr,struct directory,list)


struct super_block {
    inode_t *root;
    u32_t magic;

    u32_t modifiedTime;
    u32_t mountTime;

    u32_t freeBlockCnt;
    u32_t freeInodeCnt;

    u32_t blockSize;
    u32_t inodeSize;

    rwlock_t rwlock;
};

#endif //QUARKOS_FS_VFS_H
