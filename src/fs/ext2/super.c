//
// Created by pjs on 2021/5/26.
//

#include <types.h>
#include <fs/ext2.h>
#include <mm/kmalloc.h>
#include <fs/writeback.h>
#include <drivers/cmos.h>

#define EXT2_SB_BNO 0

#define DESCRIPTOR_PER_BLOCK(sb)    ((sb)->blockSize/sizeof(ext2_gd_t))

#define DESCRIPTOR_BID(gno, sb)     (1 + (gno) / DESCRIPTOR_PER_BLOCK(sb))

#define DESCRIPTOR_OFFSET(gno, sb)      ((gno)%DESCRIPTOR_PER_BLOCK(sb)*sizeof(ext2_gd_t))


static void check_superBlock(ext2_sb_t *sb);

ext2_sb_t *get_raw_sb(struct page**buf, super_block_t *sb) {
    struct page*tmp = ext2_block_read(EXT2_SB_BNO, sb);
    if (buf) *buf = tmp;
    return tmp->data + SECTOR_SIZE * SUPER_BLOCK_NO;
}

ext2_gd_t *get_raw_gd(struct page**buf, u32_t gno, super_block_t *sb) {
    struct page*tmp = ext2_block_read(DESCRIPTOR_BID(gno, sb), sb);
    if (buf) *buf = tmp;
    return tmp->data + DESCRIPTOR_OFFSET(gno, sb);
}


ext2_sb_info_t *superBlock_init() {
    ext2_sb_t *blk;
    ext2_gd_t *desc;
    ext2_sb_info_t *sb;
    u32_t groupCount;

    //TODO: 检测文件系统必须实现的功能
    //TODO: 检测设备和块号
    blk = page_read_no(0)->data + SECTOR_SIZE * SUPER_BLOCK_NO;
    check_superBlock(blk);

    groupCount = blk->blockCnt / blk->blockPerGroup;
    sb = kmalloc(sizeof(ext2_sb_info_t) + sizeof(struct info_descriptor) * groupCount);
    // 初始化超级块
    assertk(blk->magic == EXT2_SIGNATURE);
    sb->sb.magic = blk->magic;
    sb->sb.blockSize = (2 << (blk->logBlockSize + 9));
    sb->sb.inodeSize = blk->inodeSize;
    rwlock_init(&sb->sb.rwlock);
    sb->blockPerGroup = blk->blockPerGroup;
    sb->inodePerGroup = blk->inodePerGroup;
    sb->version = blk->fsVersionMajor;
    sb->groupCnt = blk->blockCnt / blk->blockPerGroup;

    // 初始化 info_descriptor
    for (u64_t i = 0; i < groupCount; ++i) {
        desc = get_raw_gd(NULL, i, &sb->sb);
        sb->desc[i].inodeTableAddr = desc[i].inodeTableAddr;
        sb->desc[i].blockBitmapAddr = desc[i].blockBitmapAddr;
        sb->desc[i].inodeBitmapAddr = desc[i].inodeBitmapAddr;
    }

    return sb;
}


u32_t get_free_inode_cnt(u32_t groupCnt, super_block_t *sb) {
    u64_t cnt = 0;
    ext2_gd_t *gd;
    for (u64_t i = 0; i < groupCnt; ++i) {
        gd = get_raw_gd(NULL, i, sb);
        cnt += gd->freeInodesCnt;
    }
    return cnt;
}

u32_t get_free_block_cnt(u32_t groupCnt, super_block_t *sb) {
    u64_t cnt = 0;
    ext2_gd_t *gd;
    for (u64_t i = 0; i < groupCnt; ++i) {
        gd = get_raw_gd(NULL, i, sb);
        cnt += gd->freeBlockCnt;
    }
    return cnt;
}

void ext2_write_super_block(super_block_t *_sb) {
    ext2_sb_info_t *sb = ext2_s(_sb);
    struct page*sbBuf;
    u64_t freeInodeCnt = get_free_inode_cnt(sb->groupCnt, &sb->sb);
    u64_t freeBlockCnt = get_free_block_cnt(sb->groupCnt,&sb->sb);
    ext2_sb_t *blk = get_raw_sb(&sbBuf, &sb->sb);
    assertk(blk->magic == EXT2_SIGNATURE);

    u32_t time = cur_timestamp();
    blk->freeBlockCnt = freeBlockCnt;
    blk->freeInodeCnt = freeInodeCnt;
    blk->writtenTime = time;
    mark_page_dirty(sbBuf);
}

void super_block_backup(ext2_sb_info_t *sb) {
    //备份超级块与块描述符
    u32_t bno;
    ext2_sb_t *blk;
    u32_t cur_time = cur_timestamp();
    ext2_sb_t *src = get_raw_sb(NULL, &sb->sb);
    // 备份超级块
    for (u64_t i = 1; i < sb->groupCnt; i += 2) {
        bno = sb->blockPerGroup * i;
        blk = ext2_block_read(bno, &sb->sb)->data;
        blk->writtenTime = cur_time;
        blk->freeBlockCnt = src->freeBlockCnt;
        blk->freeInodeCnt = src->freeInodeCnt;
    }

    // 备份描述符
    // TODO:
}

static void check_superBlock(ext2_sb_t *sb) {
    assertk(sb->magic == EXT2_SIGNATURE);
}