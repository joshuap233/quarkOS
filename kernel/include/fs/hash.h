//
// Created by pjs on 2021/5/22.
//

#ifndef QUARKOS_FS_HASH_H
#define QUARKOS_FS_HASH_H

#include <fs/vfs.h>

fd_t alloc_fd(inode_t *inode);

void free_fd(fd_t fd);

inode_t *get_inode(fd_t fd);

#endif //QUARKOS_FS_HASH_H
