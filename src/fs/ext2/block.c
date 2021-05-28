//
// Created by pjs on 2021/5/24.
//

#include <fs/vfs.h>
#include <fs/ext2.h>
#include <fs/writeback.h>
#include <lib/qlib.h>
#include <lib/qstring.h>

static int32_t find_block_layer(u32_t bno, u32_t id[3], u32_t n_ptr);

static u8_t get_clear_bit(u8_t ch) {
    u8_t no = 0;
    while (ch & 1) {
        ch >>= 1;
        no++;
    }
    return no;
};

// 遍历位图找到可用 bit 并设置
int32_t set_next_zero_bit(u8_t *map, u32_t max) {
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

static u32_t next_iBlock(ext2_inode_info_t *inode, u32_t bno) {
    // 遍历间接块
    u32_t *ptr;
    u32_t bid;
    int32_t layer;
    u32_t id[3];
    u32_t blockSize = inode->inode.sb->blockSize;
    u32_t n_ptr = blockSize / sizeof(u32_t);

    layer = find_block_layer(bno, id, n_ptr);

    bid = inode->blocks[N_DIRECT_BLOCK + layer];
    for (int32_t i = layer; i >= 0; --i) {
        ptr = ext2_block_read(bid, inode->inode.sb)->data;
        bid = ptr[id[i]];
    }
    return bid;
}

// 遍历直接指针与间接指针
u32_t next_block(ext2_inode_info_t *inode, u32_t bno) {
    if (bno < N_DIRECT_BLOCK)
        return inode->blocks[bno];

    return next_iBlock(inode, bno);
}


// 分配块策略:
// 如果已经分配过块,则新块与之前的块属于同一个分组,若分组已满,则分配到下一个分组
// 如果没有分配过块,则新块处于块最少的分组
static u32_t find_group(inode_t *inode) {
    ext2_sb_info_t *sb = ext2_s(inode->sb);
    ext2_inode_info_t *info = ext2_i(inode);
    u32_t groupNo, min = 0xffffffff;
    u32_t bid;
    ext2_gd_t *desc;

    // 找到需要分配的块所在的组号
    if (info->blockCnt == 0) {
        for (u64_t i = 0; i < sb->groupCnt; ++i) {
            desc = get_raw_gd(NULL, i, inode->sb);
            if (min > desc->freeBlockCnt) {
                min = desc->freeBlockCnt;
                groupNo = i;
            }
        }
    } else {
        bid = info->blocks[0];
        if (!bid) error("empty_block");
        u32_t i = groupNo = bid / sb->blockPerGroup;
        do {
            desc = get_raw_gd(NULL, groupNo, inode->sb);
            if (desc->freeBlockCnt != 0)
                break;
            groupNo = (groupNo + 1) % sb->groupCnt;
        } while (i != groupNo);
    }
    return groupNo;
}


u32_t set_block_bitmap(inode_t *inode) {
    ext2_sb_info_t *sb = ext2_s(inode->sb);
    u32_t groupNo;
    int32_t bit;
    u8_t *map;
    ext2_gd_t *desc;
    buf_t *buf, *b_buf;

    groupNo = find_group(inode);
    // 设置位图
    b_buf = ext2_block_read(sb->desc[groupNo].blockBitmapAddr, inode->sb);
    map = b_buf->data;
    bit = set_next_zero_bit(map, inode->sb->blockSize / 8);
    if (bit < 0) error("unknown_error");

    // 更新元数据
    desc = get_raw_gd(&buf, groupNo, inode->sb);
    desc->freeBlockCnt--;
    mark_page_dirty(buf);
    mark_page_dirty(b_buf);

    return bit + groupNo * sb->blockPerGroup;
}


void clear_block_bitmap(inode_t *inode, u32_t bno) {
    buf_t *d_buf, *b_buf;
    ext2_sb_info_t *sb = ext2_s(inode->sb);
    u32_t groupNo = bno / sb->blockPerGroup;

    ext2_gd_t *desc = get_raw_gd(&d_buf, groupNo, inode->sb);
    desc->freeBlockCnt++;

    b_buf = ext2_block_read(sb->desc[groupNo].blockBitmapAddr, inode->sb);
    u8_t *map = b_buf->data;
    clear_bit(&map[bno / 8], bno % 8);

    mark_page_dirty(d_buf);
    mark_page_dirty(b_buf);
}

u32_t get_data_block_cnt(inode_t *inode) {
    u32_t cnt = ext2_i(inode)->blockCnt, bno;
    u32_t ib_cnt, sqr, n_ptr;

    if (cnt < N_DIRECT_BLOCK) return cnt;

    bno = cnt - N_DIRECT_BLOCK;
    n_ptr = inode->sb->blockSize / sizeof(u32_t);
    ib_cnt = 1;
    if (bno >= n_ptr) {
        ib_cnt += n_ptr;
        bno -= n_ptr;
    };
    sqr = n_ptr * n_ptr;

    if (bno >= sqr) ib_cnt += sqr;

    return cnt - ib_cnt;
}

// 找到指针是第几重间接指针
static int32_t find_block_layer(u32_t bno, u32_t id[3], u32_t n_ptr) {
    int32_t layer = 0;
    bno -= N_DIRECT_BLOCK;
    u32_t sqr;
    if (bno >= n_ptr) {
        bno -= n_ptr;
        layer = 1;
    };
    sqr = n_ptr * n_ptr;

    if (bno >= sqr) {
        bno -= sqr;
        layer = 2;
    }
    assertk(bno < sqr * n_ptr);
    id[0] = bno % n_ptr;
    id[1] = (bno / n_ptr) % n_ptr;
    id[2] = bno / sqr;
    return layer;
}

static u32_t new_empty_block(inode_t *inode) {
    // 分配新块,并将数据置 0
    u32_t bid = set_block_bitmap(inode);
    buf_t *foo = ext2_block_read(bid, inode->sb);
    q_memset(foo->data, 0, inode->sb->blockSize);
    return bid;
}

// 使用块号(inode 对应的数据块号)找到块id(在磁盘的块编号)
u32_t get_bid(inode_t *inode, u32_t bno) {
    ext2_inode_info_t *info = ext2_i(inode);
    u32_t n_ptr = inode->sb->blockSize / sizeof(u32_t);
    u32_t id[3];
    u32_t bid, layer;

    if (bno < N_DIRECT_BLOCK) {
        return info->blocks[bno];
    }
    layer = find_block_layer(bno, id, n_ptr);
    bid = info->blocks[N_DIRECT_BLOCK + layer];
    for (int64_t i = layer; i >= 0; ++i) {
        if (!bid) break;
        u32_t *ptr = ext2_block_read(bid, inode->sb)->data;
        bid = ptr[id[i]];
    }
    return bid;
}


u32_t alloc_block(inode_t *inode, u32_t bno) {
    // 写磁盘间接指针块时, 直接写磁盘原始数据
    u32_t id[3];
    int32_t layer;
    u32_t *ptr, bid;
    buf_t *buf = NULL;
    ext2_inode_info_t *info = ext2_i(inode);
    u32_t n_ptr = inode->sb->blockSize / sizeof(u32_t);

    if (bno < N_DIRECT_BLOCK) {
        assertk(info->blocks[bno] == 0);
        bid = set_block_bitmap(inode);
        info->blockCnt++;
        info->blocks[bno] = bid;
    } else {
        layer = find_block_layer(bno, id, n_ptr);
        u32_t *_bid = &info->blocks[N_DIRECT_BLOCK + layer];
        for (int32_t i = layer; i >= -1; --i) {
            if (!(*_bid)) {
                *_bid = new_empty_block(inode);
                info->blockCnt++;
                if (i == -1) break;
            }
            buf = ext2_block_read(*_bid, inode->sb);
            ptr = buf->data;
            _bid = &ptr[id[i]];
        }
        bid = *_bid;
    }

    if (ext2_is_dir(inode)) {
        inode->size += inode->sb->blockSize;
    }
    mark_inode_dirty(inode, I_DATA);
    return bid;
}


void free_ptr(u32_t bno, u32_t layer, inode_t *inode, u32_t n_ptr) {
    if (layer == 0) {
        clear_block_bitmap(inode, bno);
        return;
    };

    u32_t *ptr = ext2_block_read(bno, inode->sb)->data;
    for (u32_t i = 0; i < n_ptr && ptr[i]; ++i) {
        free_ptr(ptr[i], layer - 1, inode, n_ptr);
    }
    clear_block_bitmap(inode, bno);
}


// 释放所有块
void free_blocks(inode_t *inode) {
    u32_t bid;
    ext2_inode_info_t *info = ext2_i(inode);
    u32_t n_ptr = inode->sb->blockSize / sizeof(u32_t);

    //释放间接指针
    for (int i = 0; i < N_DIRECT_BLOCK; ++i) {
        bid = info->blocks[i];
        if (bid) clear_block_bitmap(inode, bid);
    }

    // 回收间接指针数据块
    for (int i = 0; i < 3; ++i) {
        bid = info->blocks[N_DIRECT_BLOCK + i];
        if (bid) free_ptr(bid, i + 1, inode, n_ptr);
    }
    info->blockCnt = 0;
}
