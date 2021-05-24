//
// Created by pjs on 2021/5/24.
//

#include <fs/ext2/block.h>
#include <fs/vfs.h>
#include <fs/ext2/ext2.h>
#include <fs/page_cache.h>
#include <lib/qlib.h>
#include <lib/qstring.h>

static int32_t find_block_layer(u32_t bno, u32_t id[3]);


static u8_t get_clear_bit(u8_t ch) {
    u8_t no = 0;
    while (ch & 1) {
        ch >>= 1;
        no++;
    }
    return no;
}

// 遍历位图找到可用 bit 并设置
static int32_t set_bit_map(u8_t *map, u32_t max) {
    for (u32_t i = 0; i < max; ++i) {
        if (*map != 0xff) {
            u32_t n = get_clear_bit(*map);
            set_bit(map, n);
            return n + (i << 3);
        }
        map++;
    }
    return -1;
}


buf_t *map_block(inode_t *inode, u32_t bid, u32_t bno) {
    // 将磁盘块映射到页缓存
    buf_t *buf = ext2_block_read(bid);
    list_head_t *hdr;

    list_for_each_rev(hdr, &inode->page) {
        if (page_entry(hdr)->index < bno)
            break;
    }
    buf->index = bno;
    buf->ref_cnt++;
    list_add_next(&buf->page, hdr);
    return buf;
}


u32_t next_bno(inode_t *inode) {
    assertk(list_empty(&inode->page));
    return page_entry(&inode->page)->index + 1;
}

static u32_t iBlock_iter(u32_t bid, u32_t layer, inode_t *inode, u32_t size) {
    if (!bid) return size;
    if (layer == 0) {
        if (list_empty(&inode->page)) error("page_empty");
        map_block(inode, bid, next_bno(inode));
        return size - BLOCK_SIZE;
    };

    u32_t *ptr = ext2_block_read(bid)->data;
    for (u32_t i = 0; i < N_PTR && ptr[i] && size > 0; ++i) {
        size = iBlock_iter(ptr[i], layer - 1, inode, size);
    }
    return size;
}


void map_all_blocks(ext2_inode_info_t *info) {
    u32_t *dBlocks = info->blocks;
    u32_t size = info->inode.size;

    for (int i = 0; i < N_DIRECT_BLOCK && dBlocks[i] && size > 0; ++i) {
        map_block(&info->inode, dBlocks[i], i);
        size -= BLOCK_SIZE;
    }

    size = iBlock_iter(dBlocks[N_DIRECT_BLOCK], 1, &info->inode, size);
    size = iBlock_iter(dBlocks[N_DIRECT_BLOCK + 1], 2, &info->inode, size);
    size = iBlock_iter(dBlocks[N_DIRECT_BLOCK + 2], 3, &info->inode, size);
    assertk(size == 0);
}


// 如果已经分配过块,则新块与之前的块属于同一个分组,若分组已满,则分配到下一个分组
// 如果没有分配过块,则新块处于块最少的分组
u32_t set_block_bitmap(inode_t *inode) {
    ext2_sb_info_t *sb = ext2_s(inode->sb);
    u32_t groupNo = 0, min = 0xffffffff;
    u32_t bid;
    int32_t bit;
    u8_t *map;

    if (inode->sb->freeBlockCnt == 0) error("full_disk");

    // 找到需要分配的块所在的组号
    if (inode->pageCnt == 0) {
        for (u64_t i = 0; i < sb->groupCnt; ++i) {
            if (min > sb->desc[i].freeBlockCnt) {
                min = sb->desc[i].freeBlockCnt;
                groupNo = i;
            }
        }
    } else {
        bid = ext2_i(inode)->blocks[0];
        if (!bid) error("empty_block");
        groupNo = bid / sb->blockPerGroup;
        while (sb->desc[groupNo].freeBlockCnt == 0) {
            groupNo = (groupNo + 1) % sb->groupCnt;
        }
    }

    // 设置位图
    map = sb->desc[groupNo].blockBitmap->data;
    bit = set_bit_map(map, inode->sb->blockSize / 8);
    if (bit < 0) error("unknown_error");

    // 更新元数据
//    inode->blockCnt++;
    inode->sb->freeBlockCnt--;
    sb->desc[groupNo].freeBlockCnt--;
    return bit + groupNo * sb->blockPerGroup;
}


// inode 与同目录文件处于同一组,若分组已满,则分配到下一个分组
// 如果父目录没有文件,则 inode 处于 inode 数最少的分组
u32_t set_inode_bitmap(inode_t *parent) {
    ext2_sb_info_t *sb = ext2_s(parent->sb);
    u32_t groupNo = 0, min = 0xffffffff;
    u32_t ino;
    int32_t bit;
    u8_t *map;

    if (parent->sb->freeInodeCnt == 0) error("full_disk");

    if (!parent->dir->child) {
        for (u64_t i = 0; i < sb->groupCnt; ++i) {
            if (min > sb->desc[i].freeInodesCnt) {
                min = sb->desc[i].freeInodesCnt;
                groupNo = i;
            }
        }
    } else {
        ino = parent->dir->ino;
        groupNo = EXT2_INODE2GROUP(ino, sb);
        while (sb->desc[groupNo].freeInodesCnt == 0) {
            groupNo = (groupNo + 1) % sb->groupCnt;
        }
    }
    // TODO: 下面的元信息修改移到函数外?
    parent->sb->freeInodeCnt--;
    sb->desc[groupNo].freeInodesCnt--;
    map = sb->desc[groupNo].inodeBitmap->data;
    bit = set_bit_map(map, sb->inodePerGroup / 8);
    if (bit < 0) error("unknown_error");
    return bit + 1 + groupNo * sb->blockPerGroup;
}

void clear_inode_bitmap(inode_t *inode) {
    u32_t ino = inode->dir->ino;
    ext2_sb_info_t *sb = ext2_s(inode->sb);
    u32_t groupNo = EXT2_INODE2GROUP(ino, sb);

    u8_t *map = sb->desc[groupNo].inodeBitmap->data;
    ino--;
    clear_bit(&map[ino / 8], ino % 8);

    inode->sb->freeInodeCnt++;
    sb->desc[groupNo].freeInodesCnt++;
}


void clear_block_bitmap(inode_t *inode, u32_t bno) {
    ext2_sb_info_t *sb = ext2_s(inode->sb);
    u32_t groupNo = bno / sb->blockPerGroup;

    u8_t *map = sb->desc[groupNo].blockBitmap->data;
    clear_bit(&map[bno / 8], bno % 8);

    sb->sb.freeBlockCnt++;
    sb->desc[groupNo].freeBlockCnt++;
}

// 找到指针是第几重间接指针
static int32_t find_block_layer(u32_t bno, u32_t id[3]) {
    int32_t layer = 0;
    bno -= N_DIRECT_BLOCK;
    if (bno >= N_PTR) {
        bno -= N_PTR;
        layer = 1;
    };
    if (bno >= N_PTR * N_PTR) {
        bno -= N_PTR * N_PTR;
        layer = 2;
    }
    assertk(bno < N_PTR * N_PTR * N_PTR);
    id[0] = bno % N_PTR;
    id[1] = (bno / N_PTR) % N_PTR;
    id[2] = bno / (N_PTR * N_PTR);
    return layer;
}

static u32_t new_empty_block(inode_t *inode) {
    // 分配新块,并将数据置 0
    u32_t bid = set_block_bitmap(inode);
    buf_t *foo = ext2_block_read(bid);
    q_memset(foo->data, 0, BLOCK_SIZE);
    return bid;
}

// 使用块号(inode 对应的数据块号)找到块id(在磁盘的块编号)
u32_t get_bid(inode_t *inode, u32_t bno) {
    ext2_inode_info_t *info = ext2_i(inode);
    u32_t id[3];
    u32_t bid, layer;

    if (bno < N_DIRECT_BLOCK) {
        return info->blocks[bno];
    }
    layer = find_block_layer(bno, id);
    bid = info->blocks[N_DIRECT_BLOCK + layer];
    for (int64_t i = layer; i >= 0; ++i) {
        if (!bid) break;
        u32_t *ptr = ext2_block_read(bid)->data;
        bid = ptr[id[i]];
    }
    return bid;
}


u32_t alloc_block(inode_t *inode, u32_t bno) {
    // 写磁盘间接指针块时, 直接写磁盘原始数据
    u32_t id[3];
    int32_t layer;
    u32_t *ptr, *bid;
    buf_t *buf = NULL;
    ext2_inode_info_t *info = ext2_i(inode);

    if (bno < N_DIRECT_BLOCK) {
        if (info->blocks[bno] == 0) error("unknown_error");
        u32_t bit = set_block_bitmap(inode);
        info->blockCnt++;
        info->blocks[bno] = bit;
        return bit;
    }

    layer = find_block_layer(bno, id);

    bid = &info->blocks[N_DIRECT_BLOCK + layer];
    for (int32_t i = layer; i >= -1; --i) {
        if (!(*bid)) {
            *bid = new_empty_block(inode);
            info->blockCnt++;
            if (i == -1) break;
        }
        buf = ext2_block_read(*bid);
        ptr = buf->data;
        bid = &ptr[id[i]];
    }
    return *bid;
}


void free_ptr(u32_t bno, u32_t layer, inode_t *inode) {
    if (layer == 0) {
        clear_block_bitmap(inode, bno);
        return;
    };

    u32_t *ptr = ext2_block_read(bno)->data;
    for (u32_t i = 0; i < N_PTR && ptr[i]; ++i) {
        free_ptr(ptr[i], layer - 1, inode);
    }
    clear_block_bitmap(inode, bno);
}


// 释放所有块
void free_blocks(inode_t *inode) {
    u32_t bno;
    ext2_inode_info_t *info = ext2_i(inode);
    list_head_t *hdr;
    //释放间接指针
    for (int i = 0; i < N_DIRECT_BLOCK; ++i) {
        bno = info->blocks[i];
        if (bno) info->blocks[i] = 0;
    }

    // 回收间接指针数据块
    for (int i = 0; i < 3; ++i) {
        bno = info->blocks[N_DIRECT_BLOCK + i];
        if (bno) free_ptr(bno, i + 1, inode);
    }
    q_memset(info->blocks, 0, sizeof(u32_t) * (N_DIRECT_BLOCK + 3));
    info->blockCnt = 0;

    list_for_each(hdr, &inode->page) {
        page_entry(hdr)->ref_cnt--;
    }
    list_header_init(&inode->page);
}