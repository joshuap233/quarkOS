//
// Created by pjs on 2021/5/13.
//

#ifndef QUARKOS_FS_VFS_H
#define QUARKOS_FS_VFS_H

#include <types.h>
#include <lib/list.h>
#include <lib/rwlock.h>
#include <mm/page.h>
#include <lib/queue.h>

typedef struct super_block super_block_t;
typedef struct inode inode_t;
typedef inode_t fd_t;
typedef struct directory directory_t;

#define FILE_NAME_LEN 128
#define FILE_DNAME_LEN 40

enum SEEK_WHENCE {
    SEEK_SET = 0,  // 参数 offset 即为新的读写位置.
    SEEK_CUR,      // 读写位置往后增加 offset 字节.
    SEEK_END       // 文件尾后再增加 offset 个字节
};


// 所有文件系统必须支持的操作
struct fs_ops {
    directory_t *(*find)(directory_t *parent, const char *name);

    int32_t (*open)(inode_t *inode);

    int32_t (*close)(inode_t *inode);

    u32_t (*read)(inode_t *file, uint32_t offset, uint32_t size, char *buf);

    u32_t (*write)(inode_t *file, uint32_t offset, uint32_t size, char *buf);

    int32_t (*mkdir)(inode_t *parent, const char *name);

    int32_t (*mkfile)(inode_t *parent, const char *name);

    int32_t (*rmdir)(inode_t *dir);

    int32_t (*link)(inode_t *src, inode_t *parent, const char *name);

    int32_t (*unlink)(directory_t *file);

    void (*ls)(inode_t *dir);

    void (*write_back)(inode_t *inode);

    void (*write_super_block)(super_block_t *sb);
};


// vfs 支持的操作
struct vfs_ops {
    inode_t *(*open)(const char *path);

    int32_t (*close)(inode_t *node);

    u32_t (*read)(inode_t *node, uint32_t size, char *buf);

    u32_t (*write)(inode_t *node, uint32_t size, char *buf);

    int32_t (*lseek)(inode_t *node, int32_t offset, enum SEEK_WHENCE whence);

    int32_t (*mkdir)(const char *path);

    int32_t (*rmdir)(const char *path);

    int32_t (*mkfile)(const char *path);

    int32_t (*unlink)(const char *path);

    int32_t (*link)(const char *src, const char *desc);

    void (*ls)(const char *path);

    void (*umount)();

    void (*mount)(char *path, inode_t *inode);
};

extern struct vfs_ops vfs_ops;

enum inode_state {
    I_OLD = 0, I_NEW, I_DATA, I_TIME, I_DEL
};

extern void mark_inode_dirty(inode_t *inode, enum inode_state state);

// vfs 没有统一的 mode 与 type,
// 因此 mode 与 type 由具体的文件系统管理
struct inode {
    enum inode_state state;

    u16_t refCnt;
    u16_t linkCnt;

    u16_t permission;
    u16_t type;
    u64_t size;            // 文件大小

    u32_t createTime;
    u32_t accessTime;
    u32_t modifiedTime;
    u32_t deleteTime;

    u32_t offset;         // 用于常规文件读写

    list_head_t lru;
    lfq_node dirty;

    struct fs_ops *ops;
    struct directory *dir;
    struct super_block *sb;
    rwlock_t rwlock;
};

#define inode_dirty_entry(ptr) list_entry(ptr,inode_t,dirty)
#define inode_lru_entry(ptr)    list_entry(ptr,inode_t,lru)

struct directory_ops {
    // 将 inode 复制到 dir->inode
    void (*inode_mount)(directory_t *dir);

};

struct directory {
    u32_t ino;
    inode_t *inode;

    //超过 40 字节的名称存储在 h_name
    char l_name[FILE_DNAME_LEN];
    char *h_name;
    u8_t nameLen;

    struct super_block *sb;
    struct directory_ops *ops;
    struct directory *parent;
    list_head_t child;    // 子目录
    list_head_t brother;  // 同层目录
    list_head_t link;     // 引用相同 inode 的目录链表
    rwlock_t rwLock;
};

#define dir_brother_entry(ptr)    list_entry(ptr,directory_t,brother)
#define dir_child_entry(ptr)      list_entry(ptr,directory_t,child)
#define dir_link_entry(ptr)       list_entry(ptr,directory_t,link)


struct super_block {
    inode_t *root;
    u32_t magic;

    u32_t blockSize;
    u32_t inodeSize;

    rwlock_t rwlock;
};

bool dir_name_cmp(directory_t *dir, const char *name);

void dir_name_set(directory_t *dir, const char *name, u32_t len);

char *dir_name_dump(directory_t *dir);

#define SEPARATOR            '/'

#endif //QUARKOS_FS_VFS_H
