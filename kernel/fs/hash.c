//
// Created by pjs on 2021/5/22.
//
// fd 索引 inode 与 inode 反向索引 fd
#include <fs/hash.h>
#include <fs/vfs.h>
#include <mm/kmalloc.h>
#include <highmem.h>

#define FILE_TABLE_SIZE  1024
#define FILE_HASH_SIZE   1031
#define FILE_HASH_SQRT   32

static struct hash {
    // inode 索引 fd
    struct hash_entry {
        inode_t *key;
        fd_t value;
    } entry[FILE_HASH_SIZE];
    // fd 索引 inode
    inode_t *file_table[FILE_TABLE_SIZE];
} hash = {
        .entry = {[0 ...FILE_HASH_SIZE - 1]={
                .key =NULL,
                .value=0
        }},
        .file_table = {
                [0 ...FILE_TABLE_SIZE - 1] = NULL
        }
};

INLINE u32_t hash_generate(inode_t *inode) {
    ptr_t ptr = (ptr_t) inode;
    assertk(ptr > KERNEL_START);

    u32_t index = (ptr - KERNEL_START) % FILE_HASH_SIZE;
    return index;
}

// 使用平方探测法解决冲突
INLINE u32_t hash_conflict(u32_t index, int i) {
    return (index + i * i) % FILE_HASH_SIZE;
}

static void hash_insert(inode_t *inode, fd_t fd) {
    u32_t _hash = hash_generate(inode);
    u32_t index;

    for (int i = 0; i <= FILE_HASH_SQRT; ++i) {
        index = hash_conflict(_hash, i);
        if (hash.entry[index].key == NULL) {
            hash.entry[index].key = inode;
            hash.entry[index].value = fd;
            return;
        }
    }
    assertk(0);
}


static struct hash_entry *hash_findf(inode_t *inode) {
    u32_t _hash = hash_generate(inode);
    u32_t index;

    for (int i = 0; i <= FILE_HASH_SQRT; ++i) {
        index = hash_conflict(_hash, i);
        if (hash.entry[index].key == inode) {
            return &hash.entry[index];
        }
    }
    return NULL;
}

static inode_t *hash_findi(fd_t fd) {
    inode_t *inode = hash.file_table[fd];
    return inode;
}


static void hash_del(inode_t *inode) {
    struct hash_entry *entry = hash_findf(inode);
    assertk(entry);

    entry->key = NULL;
}

void free_fd(fd_t fd) {
    assertk(fd >= 0);

    inode_t *inode = hash.file_table[fd];
    assertk(inode != NULL);

    hash.file_table[fd] = 0;
    hash_del(inode);
}


fd_t alloc_fd(inode_t *inode) {
    assertk(inode != NULL);

    struct hash_entry *entry = hash_findf(inode);
    if (entry) return entry->value;

    for (int i = 0; i < FILE_TABLE_SIZE; ++i) {
        if (hash.file_table[i] == 0) {
            hash.file_table[i] = inode;
            hash_insert(inode, i);
            return i;
        }
    }
    assertk(0);
    return -1;
}

inode_t *get_inode(fd_t fd) {
    return hash_findi(fd);
}