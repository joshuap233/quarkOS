//
// Created by pjs on 2021/4/20.
//
// fat 文件系统
#include "fs/ext2.h"
#include "fs/bio.h"
#include "types.h"
#include "lib/qlib.h"


typedef struct block {
    struct block_data {
        buf_t *buf;
        struct block *next;
    } data;
    uint32_t no;      // 块编号, 从 0 开始
} block_t;

// TODO: 我需要一个固定大小的块分配器
struct block_data blockData[100];

typedef struct node {
    buf_t *buf;
    uint16_t offset;     // 占用的 buf 数据块偏移
    uint32_t no;         // inode 号, 从 1 开始
} node_t;

static superBlock_t *suBlock;
static uint32_t blockSize, groupCnt;
static groupDesc_t *descriptor;
static node_t root;

#define BLOCK2LBA(block_no) ((block_no) * blockSize / SECTOR_SIZE)
#define INO2LBA(inodeTableAddr, no)  (BLOCK2LBA(inodeTableAddr) + suBlock->inodeSize * ((no) - 1) / SECTOR_SIZE)
#define GET_INODE(node)      ((inode_t *)((node).buf->data + (node).offset))

void ext2_readInode(struct node *node, uint32_t no, groupDesc_t *desc);

void ext2_init() {
    buf_t *buf = bio_get(2), *buf2;
    bio_read(buf);

    suBlock = (superBlock_t *) buf->data;
    assertk(suBlock->magic == EXT2_SIGNATURE);
    blockSize = 2 << (suBlock->logBlockSize + 9);
    groupCnt = suBlock->blockCnt / suBlock->blockPerGroup;
    assertk(groupCnt == suBlock->inodeCnt / suBlock->inodePerGroup);
    // 懒得解析 > SECTOR_SIZE 的情况....
    assertk(suBlock->inodeSize <= SECTOR_SIZE);

    buf2 = bio_get(BLOCK2LBA(blockSize == 1024 ? 2 : 1));
    bio_read(buf2);
    descriptor = (groupDesc_t *) buf2->data;

    ext2_readInode(&root, ROOT_INUM, descriptor);
    inode_t *inode = GET_INODE(root);
    assertk((inode->mode & MOD_DIR) == MOD_DIR);
}

// no 为 inode 在当前组的索引,
void ext2_readInode(node_t *node, uint32_t no, groupDesc_t *desc) {
    node->no = no;
    node->buf = bio_get(INO2LBA(desc->inodeTableAddr, no));
    node->offset = suBlock->inodeSize * (node->no - 1) % SECTOR_SIZE;
    bio_read(node->buf);
}

void ext2_readBlock(block_t *block, uint32_t no) {

}

void ext2_mkdir(char *name) {

}

void ext2_mkfile() {

}

void ext2_unlink() {

}

void ext2_listdir() {

}

void ext2_open() {

}

void ext2_close() {

}
