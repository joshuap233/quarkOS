//
// Created by pjs on 2021/5/24.
//

#ifndef QUARKOS_FS_EXT2_BLOCK_H
#define QUARKOS_FS_EXT2_BLOCK_H

#include <fs/vfs.h>
#include <fs/ext2/ext2.h>

buf_t *map_block(inode_t *inode, u32_t bid, u32_t bno);

void map_all_blocks(ext2_inode_info_t *info);

u32_t set_inode_bitmap(inode_t *parent);

u32_t set_block_bitmap(inode_t *inode);

void clear_inode_bitmap(inode_t *inode);

void clear_block_bitmap(inode_t *inode, u32_t bno);

u32_t next_bno(inode_t *inode);

u32_t alloc_block(inode_t *inode, u32_t bno);

void free_blocks(inode_t *inode);

u32_t get_bid(inode_t *inode, u32_t bno);

#endif //QUARKOS_FS_EXT2_BLOCK_H
