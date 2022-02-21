//
// Created by pjs on 2021/5/26.
//
#include <types.h>
#include <fs/vfs.h>
#include <fs/ext2.h>
#include <fs/writeback.h>
#include <mm/slab.h>
#include <lib/qstring.h>
#include <drivers/cmos.h>

static u32_t set_inode_bitmap(inode_t *parent);

static void clear_inode_bitmap(inode_t *inode);

static struct slabCache inoCache;

LIST_HEAD(inode_lru);
static struct spinlock lru_lock;

void ext2_inode_init() {
    spinlock_init(&lru_lock);
    cache_alloc_create(&inoCache, sizeof(ext2_inode_info_t));
}

static ext2_inode_t *get_raw_inode(struct page **buf, super_block_t *_sb, u32_t ino) {
    ext2_sb_info_t *sb = ext2_s(_sb);
    u32_t bIno = (ino - 1) % sb->inodePerGroup;;// inode 块内编号
    u32_t inodePerBlock = _sb->blockSize / _sb->inodeSize;
    u32_t base = sb->desc[EXT2_INODE2GROUP(ino, sb)].inodeTableAddr;
    u32_t blk_no = base + bIno / inodePerBlock;
    struct page *page = ext2_block_read(blk_no, _sb);
    if (buf)*buf = page;
    return page->data + bIno * sb->sb.inodeSize;
}

static ext2_inode_info_t *inode_get() {
    ext2_inode_info_t *info = cache_alloc(&inoCache);
    if (!info) return NULL;
    spinlock_lock(&lru_lock);
    list_add_prev(&info->inode.lru, &inode_lru);
    spinlock_unlock(&lru_lock);
    return info;
}

static void inode_destroy(inode_t *inode) {
    ext2_inode_info_t *info = ext2_i(inode);
    cache_free(&inoCache, info);
}

inode_t *inode_cpy(u32_t ino, super_block_t *sb) {
    // 拷贝磁盘原始数据到 vfs 缓存
    ext2_inode_t *ei = get_raw_inode(NULL, sb, ino);
    ext2_inode_info_t *info = inode_get();
    if (!info) return NULL;
    inode_t *inode = &info->inode;
    info->blockCnt = ei->cntSectors / 8;

    inode->refCnt = 0;
    inode->linkCnt = ei->linkCnt;

    inode->permission = ei->mode & 0xfff;
    inode->type = ei->mode & (~0xfff);
    inode->size = ei->size;

    inode->createTime = ei->createTime;
    // 将 inode 缓存到 cache 时,更新访问时间
    inode->accessTime = cur_timestamp();
    inode->modifiedTime = ei->modTime;
    inode->deleteTime = ei->delTime;

    inode->offset = 0;
    list_header_init(&inode->lru);
    lfQueue_node_init(&inode->dirty);

    inode->ops = &ext2_ops;
    inode->sb = sb;
    inode->dir = NULL;
    rwlock_init(&inode->rwlock);

    // info 信息
    memcpy(info->blocks, ei->blocks, sizeof(u32_t) * N_BLOCKS);
    info->blockCnt = ei->cntSectors / (sb->blockSize / SECTOR_SIZE);

    mark_inode_dirty(inode, I_TIME);
    return inode;
}


inode_t *inode_alloc(inode_t *parent, u16_t mode, const char *name) {
    // 初始化新分配的 inode
    u32_t linkCnt, groupNo;
    ext2_gd_t *desc;
    struct page *buf;
    bool isDir = false;

    super_block_t *sb = parent->sb;
    ext2_inode_info_t *info = inode_get();

    inode_t *inode = &info->inode;
    u32_t ino = set_inode_bitmap(parent);
    directory_t *dir = directory_alloc(name, ino, parent->dir, inode);

    u32_t time = cur_timestamp();

    switch (mode & (~0xfff)) {
        case EXT2_IFREG:
            linkCnt = 1;
            break;
        case EXT2_IFDIR:
            isDir = true;
            linkCnt = 2;
            break;
        default: error("unknown_type");
    }

    inode->refCnt = 0;
    inode->linkCnt = linkCnt;

    inode->permission = mode & 0xfff;
    inode->type = mode & (~0xfff);
    inode->size = 0;

    inode->createTime = time;
    inode->accessTime = time;
    inode->modifiedTime = time;
    inode->deleteTime = 0;

    inode->offset = 0;

    list_header_init(&inode->lru);
    lfQueue_node_init(&inode->dirty);

    inode->ops = &ext2_ops;
    inode->dir = dir;
    inode->sb = sb;

    rwlock_init(&inode->rwlock);

    // info 信息
    bzero(info->blocks, sizeof(u32_t) * N_BLOCKS);
    info->blockCnt = 0;

    // 修改元数据
    groupNo = EXT2_INODE2GROUP(ino, ext2_s(inode->sb));

    desc = get_raw_gd(&buf, groupNo, sb);
    desc->freeInodesCnt--;
    if (isDir) desc->dirNum++;

    mark_page_dirty(buf);
//    mark_inode_dirty(inode, I_NEW);
    return inode;
}

static void clear_inode_bitmap(inode_t *inode) {
    // 不会修改组的元数据
    u32_t ino = inode->dir->ino;
    ext2_sb_info_t *sb = ext2_s(inode->sb);
    u32_t groupNo = EXT2_INODE2GROUP(ino, sb);
    struct page *buf;


    buf = ext2_block_read(sb->desc[groupNo].inodeBitmapAddr, inode->sb);
    u8_t *map = buf->data;
    ino--;
    clear_bit(&map[ino / 8], ino % 8);

    mark_page_dirty(buf);
}


// 分配 inode 策略:
// inode 与同目录文件处于同一组,若分组已满,则分配到下一个分组
// 如果父目录没有文件,则 inode 处于 inode 数最少的分组
static u32_t find_group(inode_t *parent) {
    ext2_sb_info_t *sb = ext2_s(parent->sb);
    u32_t groupNo, min = 0xffffffff;
    ext2_gd_t *desc;
    u32_t ino;

    if (dir_empty(parent)) {
        for (u64_t i = 0; i < sb->groupCnt; ++i) {
            desc = get_raw_gd(NULL, i, &sb->sb);
            if (min > desc->freeInodesCnt) {
                min = desc->freeInodesCnt;
                groupNo = i;
            }
        }
    } else {
        ino = parent->dir->ino;
        groupNo = EXT2_INODE2GROUP(ino, sb);
        u32_t i = groupNo;
        do {
            desc = get_raw_gd(NULL, groupNo, &sb->sb);
            if (desc->freeInodesCnt != 0)
                break;
            groupNo = (groupNo + 1) % sb->groupCnt;
        } while (i != groupNo);
    }
    return groupNo;
}


static u32_t set_inode_bitmap(inode_t *parent) {
    ext2_sb_info_t *sb = ext2_s(parent->sb);
    u32_t groupNo;
    int32_t bit;
    u8_t *map;
    struct page *buf;

    groupNo = find_group(parent);

    buf = ext2_block_read(sb->desc[groupNo].inodeBitmapAddr, parent->sb);
    // 设置位图
    map = buf->data;
    bit = set_next_zero_bit(map, sb->inodePerGroup / 8);
    if (bit < 0) error("unknown_error");

    mark_page_dirty(buf);
    return bit + 1 + groupNo * sb->blockPerGroup;
}

void inode_delete(inode_t *inode) {
    u32_t groupNo;
    struct page *buf;
    u32_t time = cur_timestamp();
    ext2_gd_t *desc;

    wlock_lock(&inode->rwlock);
    u32_t isDir = ext2_is_dir(inode);

    // 清空数据块
    free_blocks(inode);
    inode->linkCnt = 0;
    inode->deleteTime = time;

    clear_inode_bitmap(inode);

    list_del(&inode->dir->brother);

    // 修改元数据
    groupNo = EXT2_INODE2GROUP(inode->dir->ino, ext2_s(inode->sb));
    desc = get_raw_gd(&buf, groupNo, inode->sb);
    desc->freeInodesCnt++;

    if (isDir) desc->dirNum--;

    mark_inode_dirty(inode, I_DEL);
    mark_page_dirty(buf);
}

void ext2_write_back(inode_t *inode) {
    struct page *buf;
    ext2_inode_t *ino = get_raw_inode(&buf, inode->sb, inode->dir->ino);

    if (inode->state == I_TIME || inode->state == I_NEW) {
        ino->accessTime = inode->accessTime;
        ino->delTime = inode->deleteTime;
        ino->modTime = inode->modifiedTime;
        ino->createTime = inode->createTime;
    }

    if (inode->state == I_DATA || inode->state == I_NEW) {
        ino->size = inode->size;
        ino->dirAcl = inode->size >> 32;
        ino->linkCnt = inode->linkCnt;
        ino->cntSectors = ext2_i(inode)->blockCnt * 8;
        memcpy(ino->blocks, ext2_i(inode)->blocks, N_BLOCKS * sizeof(u32_t));
        ino->mode = inode->type | inode->permission;
    }
    if (inode->state == I_DEL) {
        assertk(inode->linkCnt == 0);
        ino->linkCnt = 0;
        ino->delTime = inode->deleteTime;
        ino->cntSectors = 0;
        bzero(ino->blocks, N_BLOCKS * sizeof(u32_t));
        directory_destroy(inode->dir);
        inode_destroy(inode);
    } else {
        inode->state = I_OLD;
    }
    mark_page_dirty(buf);
}

UNUSED void inode_recycle() {
    //TODO: 回收 inode 与 directory
}