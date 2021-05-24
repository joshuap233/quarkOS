//
// Created by pjs on 2021/5/23.
//
#include "fs/ext2/write_back.h"
#include "fs/ext2/ext2.h"
#include "fs/page_cache.h"
#include "types.h"
#include "lib/qlib.h"
#include "fs/buf.h"
#include "fs/vfs.h"
#include "lib/qstring.h"
#include "drivers/cmos.h"
#include "mm/kmalloc.h"
#include "sched/kthread.h"
#include "mm/slab.h"

/*---  目录项操作 ---*/
ext2_dir_t *next_entry(ext2_dir_t *dir);

INLINE bool dir_empty(ext2_inode_t *inode);

static ext2_dir_t *new_dir_entry(ext2_dir_t *dir, u32_t ino, char *name, u8_t type);

static ext2_dir_t *find_dir_entry(buf_t **buf, ext2_inode_t *parent, char *name);

static void create_dir_entry(ext2_inode_t *inode, char *name, u32_t ino, u8_t type);

static void del_dir_entry(ext2_dir_t *dir);

static void merge_dir_entry(ext2_dir_t *dir);

/*--- 数据块操作 ---*/

static u32_t find_blk_id(ext2_inode_t *inode, u32_t offset);

static u32_t next_block(ext2_inode_t *inode, u32_t bno);

static void free_block(u32_t ino, u32_t bno);

static u32_t new_block(u32_t ino);

static u32_t new_empty_block(u32_t ino);

static u32_t alloc_block(ext2_inode_t *inode, u32_t ino);

static u32_t nextFreeBlk(ext2_inode_t *inode);

/*--- inode 操作 ---*/
static u32_t new_inode(u32_t ino);

static void *get_inode(buf_t **buf, u32_t ino);

static void free_inode(u32_t ino);

static void del_inode(ext2_inode_t *inode, u32_t ino);

static void update_inode_size(ext2_inode_t *inode, int32_t change);

/*--- 组与超级块数据操作  ---*/

//static void sb_backup();

static void write_superBlock();

static ext2_sb_t *get_superBlock();

static void *get_descriptor(u32_t ino);

//static void descriptor_backup(u32_t ino);

static void write_descriptor(u32_t ino);

/*--- 其他  ---*/
static u64_t file_size(ext2_inode_t *inode);

static u8_t get_clear_bit(u8_t ch);

static int32_t map_set_bit(u8_t *map, u32_t max);

static inode_t *inode2vnode(ext2_inode_t *inode, ext2_dir_t *dir);


// inode 的块号
#define INODE_BID(base, ino)    ((base)+((ino)-1)%inodePerGroup / INODE_PER_BLOCK)
// inode 在当前块中偏移
#define INODE_OFFSET(no)        ((((no)-1) % INODE_PER_BLOCK) * INODE_SIZE)

#define INODE2GROUP(ino, sb)        (((ino)-1) / inodePerGroup)

#define DESCRIPTOR_OFFSET(ino)  ((((ino)-1) / inodePerGroup) * sizeof(groupDesc_t))

#define DESCRIPTOR_BID(ino)     (1 + INODE2GROUP(ino) / DESCRIPTOR_PER_BLOCK)

#define BLOCK2LBA(block_no)     (BLOCK_SIZE / SECTOR_SIZE * (block_no))
// 目录项四字节对齐
#define ALIGN4(size)            MEM_ALIGN(size, 4)

#define dir_len(dir)            (ALIGN4(sizeof(ext2_dir_t) + (dir)->nameLen))

#define block_read(blk_no)      page_read_no(BLOCK2LBA(blk_no))

#define block_write(buf)        page_write(buf)

#define DIR_END(dir)            (((dir) & (~(BLOCK_SIZE-1))) + BLOCK_SIZE)

#define for_each_block(bid, inode)        for(u32_t bno=0;((bid) = next_block(inode, bno++));)

#define for_each_dir(dir)                 for (ext2_dir_t* end = (void *) DIR_END((ptr_t)(dir)); (dir) < end; (dir) = next_entry(dir))

#define for_each_group(ino)               for (; (ino) <= groupCnt * inodePerGroup; (ino) += inodePerGroup)


void ext2_mkdir(inode_t *parent, const char *name) {
    buf_t *pBuf;
    ext2_inode_t *pInode = get_inode(&pBuf, parent->ino);
    ext2_dir_t *target = find_dir_entry(NULL, pInode, name);
    if (target)return;

    // 设置新 inode 与目录项
    buf_t *buf;
    u32_t cur_time = cur_timestamp();
    u32_t ino = new_inode(parent->ino);
    u32_t blk_no = new_block(parent->ino);
    ext2_inode_t *inode = get_inode(&buf, ino);
    q_memset(inode, 0, sizeof(ext2_inode_t));
    inode->blocks[0] = blk_no;
    inode->createTime = cur_time;
    inode->accessTime = cur_time;
    inode->modTime = cur_time;
    inode->size = BLOCK_SIZE;
    inode->linkCnt = 2;
    inode->mode = EXT2_ALL_RWX | EXT2_IFDIR;
    inode->cntSectors = SECTOR_PER_BLOCK;
    block_write(buf);

    groupDesc_t *desc = get_descriptor(ino);
    desc->dirNum++;
    write_descriptor(ino);

    buf_t *buf1 = block_read(blk_no);
    ext2_dir_t *dir = buf1->data;
    dir->inode = 0;
    dir->entrySize = BLOCK_SIZE;
    dir = new_dir_entry(dir, ino, ".", EXT2_FT_DIR);
    new_dir_entry(dir, parent->ino, "..", EXT2_FT_DIR);
    block_write(buf1);


    create_dir_entry(pInode, name, ino, EXT2_FT_DIR);
    // 修改 parent inode
    pInode->accessTime = cur_time;
    pInode->modTime = cur_time;
    pInode->linkCnt++;
    block_write(pBuf);
}


void ext2_mkfile(inode_t *parent, const char *name) {
    buf_t *pBuf;
    ext2_inode_t *pInode = get_inode(&pBuf, parent->ino);
    ext2_dir_t *target = find_dir_entry(NULL, pInode, name);
    if (target)return;

    // 设置新 inode 与目录项
    buf_t *buf;
    u32_t cur_time = cur_timestamp();
    u32_t ino = new_inode(parent->ino);
    ext2_inode_t *inode = get_inode(&buf, ino);
    q_memset(inode, 0, sizeof(ext2_inode_t));
    inode->createTime = cur_time;
    inode->accessTime = cur_time;
    inode->modTime = cur_time;
    inode->linkCnt = 1;
    inode->mode = EXT2_ALL_RWX | EXT2_IFREG;
    block_write(buf);

    create_dir_entry(pInode, name, ino, EXT2_FT_REG_FILE);
    // 修改 parent inode
    pInode->accessTime = cur_time;
    block_write(pBuf);
}


// 文件操作,文件硬链接数为 1, 删除文件
void ext2_unlink(inode_t *file) {
//    assertk(parent && name);
//    u32_t cur_time;
//    buf_t *pBuf, *buf, *tBuf;
//
//    // 查找
//    ext2_inode_t *pInode = get_inode(&pBuf, parent->ino);
//    ext2_dir_t *target = find_dir_entry(&tBuf, pInode, name);
//    if (target == NULL) assertk(0);
//    u32_t ino = target->inode;
//
//    assertk(target->type = EXT2_FT_REG_FILE);
//
//    // 找到对应的 inode
//    ext2_inode_t *inode = get_inode(&buf, ino);
//    inode->linkCnt--;
//    assertk(inode->linkCnt >= 0);
//
//    del_dir_entry(target);
//    if (inode->linkCnt == 0) {
//        del_inode(inode, ino);
//        cur_time = cur_timestamp();
//        pInode->modTime = cur_time;
//        block_write(pBuf);
//        block_write(tBuf);
//    }
//
//    block_write(buf);
//
//    // 同步元数据
//    ext2_sb_t *blk = get_superBlock();
//    blk->writtenTime = cur_time;
//    write_superBlock();
}


inode_t *ext2_find(inode_t *cwd, const char *_path) {
    char *path = q_strdup(_path);
    buf_t *buf1, *buf2, *buf3;
    ext2_inode_t *inode = get_inode(&buf1, cwd->ino);
    ext2_dir_t *dir;
    for (int i = 0; path[i]; ++i) {
        if (path[i] == SEPARATOR) {
            path[i] = SEPARATOR;
            dir = find_dir_entry(&buf2, inode, path);
            assertk(dir);
            inode = get_inode(&buf3, dir->inode);
            path = &path[i] + 1;
            i = 0;
        }
    }
    assertk(dir);
    kfree(path);
    return inode2vnode(inode, dir);
}

inode_t *get_root() {
    buf_t *buf, *buf1;
    ext2_inode_t *inode = get_inode(&buf, ROOT_INUM);
    ext2_dir_t *dir = find_dir_entry(&buf1, inode, ".");
    assertk(dir);
    return inode2vnode(inode, dir);
}


INLINE bool dir_empty(ext2_inode_t *inode) {
    assertk(inode->mode & EXT2_IFDIR);
    return inode->linkCnt == 2;
}

void ext2_rmdir(inode_t *dir) {
//    assertk(parent && name);
//    u32_t cur_time;
//    buf_t *pBuf, *iBuf, *tBuf;
//
//    // 查找
//    ext2_inode_t *pInode = get_inode(&pBuf, parent->ino);
//    ext2_dir_t *target = find_dir_entry(&tBuf, pInode, name);
//    if (target == NULL) assertk(0);
//    assertk(target->type == EXT2_FT_DIR);
//    u32_t ino = target->inode;
//
//    // 找到对应的 inode ,并判断目录是否为空
//    ext2_inode_t *inode = get_inode(&iBuf, ino);
//    assertk(dir_empty(inode));
//
//    del_inode(inode, ino);
//
//    // 删除目录项
//    del_dir_entry(target);
//    block_write(tBuf);
//    block_write(iBuf);
//
//    cur_time = cur_timestamp();
//    pInode->accessTime = cur_time;
//    pInode->modTime = cur_time;
//    pInode->linkCnt--;
//    block_write(pBuf);
//
//    // 同步元数据
//    groupDesc_t *desc = get_descriptor(ino);
//    desc->dirNum--;
//    write_descriptor(ino);
//
//    ext2_sb_t *blk = get_superBlock();
//    blk->writtenTime = cur_time;
//    write_superBlock();
}

// 硬链接不能指向目录
void ext2_link(inode_t *src, inode_t *parent, const char *name) {
//    buf_t *pBuf1, *pBuf2, *tBuf1, *tBuf2, *tgBuf;
//    ext2_inode_t *p1Inode = get_inode(&pBuf1, parent->ino);
//    ext2_dir_t *target = find_dir_entry(&tBuf1, p1Inode, target_name);
//    if (!target || (target->type & EXT2_FT_DIR)) return;
//
//    ext2_inode_t *p2Inode = get_inode(&pBuf2, parent2->ino);
//    ext2_dir_t *foo = find_dir_entry(&tBuf2, p2Inode, new_name);
//    if (foo) return;
//
//    u32_t cur_time = cur_timestamp();
//
//    ext2_inode_t *tin = get_inode(&tgBuf, target->inode);
//    tin->linkCnt++;
//    tin->modTime = cur_time;
//
//    p1Inode->accessTime = cur_time;
//    p2Inode->accessTime = cur_time;
//    p2Inode->modTime = cur_time;
//
//    create_dir_entry(p2Inode, new_name, target->inode, target->type);
//    block_write(pBuf1);
//    block_write(pBuf2);
//    block_write(tgBuf);
}

void ext2_chmod() {

}

// 打印 parent 目录内容, debug 用
void ext2_ls(inode_t *parent) {
    buf_t *buf, *buf1;
    ext2_dir_t *dir;
    ext2_inode_t *inode = get_inode(&buf, parent->ino);

    u32_t bid;

    for_each_block(bid, inode) {
        buf1 = block_read(bid);
        dir = (ext2_dir_t *) buf1->data;
        for_each_dir(dir) {
            if (dir->inode != 0) {
                prints((void *) dir->name, dir->nameLen);
                printfk("\n");
            }
        }
    }
}

void ext2_open(inode_t *file) {
    buf_t *buf;
    ext2_inode_t *inode = get_inode(&buf, file->ino);
    inode->accessTime = cur_timestamp();
    block_write(buf);
}


u32_t ext2_read(inode_t *file, uint32_t offset, uint32_t size, char *buf) {
    buf_t *iBuf, *dBuf;
    ext2_inode_t *inode;
    u32_t rdSize, blkOffset, bid;
    u64_t end;

    inode = get_inode(&iBuf, file->ino);
    assertk(inode->mode & EXT2_IFREG);
    end = file_size(inode);
    if (offset > end) return 0;

    if (offset + size > end)
        size = end - offset;

    while (size > 0) {
        blkOffset = offset % BLOCK_SIZE;      // 块内偏移
        rdSize = (size < BLOCK_SIZE ? size : BLOCK_SIZE) - blkOffset;
        bid = find_blk_id(inode, offset);
        assertk(bid);

        dBuf = block_read(bid);
        q_memcpy(buf, dBuf->data + blkOffset, rdSize);

        size -= rdSize;
        buf += rdSize;
        offset += rdSize;
    }

    u32_t cur_time = cur_timestamp();
    inode->accessTime = cur_time;
    block_write(iBuf);
    return size;
}

u32_t ext2_write(inode_t *file, uint32_t offset, uint32_t size, char *buf) {
    buf_t *iBuf, *dBuf;
    ext2_inode_t *inode;
    u32_t wtSize, blkOffset, bid;
    u32_t end, fileSize;
    int64_t overflow;
    u32_t cur_time;
    u32_t ret = size;

    inode = get_inode(&iBuf, file->ino);
    assertk(inode->mode & EXT2_IFREG);
    fileSize = file_size(inode);
    end = offset + size;

    overflow = end - nextFreeBlk(inode) * BLOCK_SIZE;

    if (end > file_size(inode))
        update_inode_size(inode, end - fileSize);

    for (; overflow >= 0; overflow -= BLOCK_SIZE) {
        alloc_block(inode, file->ino);
    }

    while (size > 0) {
        blkOffset = offset % BLOCK_SIZE;      // 块内偏移
        wtSize = size < BLOCK_SIZE ? size : BLOCK_SIZE - blkOffset;
        bid = find_blk_id(inode, offset);
        assertk(bid);

        dBuf = block_read(bid);
        q_memcpy(dBuf->data + blkOffset, buf, wtSize);
        block_write(dBuf);

        size -= wtSize;
        buf += wtSize;
        offset += wtSize;
    }

    cur_time = cur_timestamp();
    inode->accessTime = cur_time;
    inode->modTime = cur_time;
    block_write(iBuf);
    return ret;
}

void ext2_close(inode_t *inode) {}


// 使用文件偏移找到直接指针
static u32_t find_blk_id(ext2_inode_t *inode, u32_t offset) {
    u32_t bid = next_block(inode, offset / BLOCK_SIZE);
    return bid;
}

static void update_inode_size(ext2_inode_t *inode, int32_t change) {
    u64_t size = file_size(inode);
    if (change < 0) assertk(size >= BLOCK_SIZE);
    size += change;
    if (version >= 1 && (inode->mode & EXT2_IFREG)) {
        inode->dirAcl = size >> 32;
    }
    inode->size = size & MASK_U64(32);
}

static u64_t file_size(ext2_inode_t *inode) {
    if (version >= 1 && (inode->mode & EXT2_IFREG)) {
        return ((u64_t) inode->dirAcl << 32) + inode->size;
    }
    return inode->size;
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

static u32_t next_iBlock(ext2_inode_t *inode, u32_t bno) {
    // 遍历间接块
    u32_t *ptr;
    u32_t bid;
    int32_t layer;
    u32_t id[3];
    layer = find_block_layer(bno, id);

    bid = inode->blocks[N_DIRECT_BLOCK + layer];
    for (int32_t i = layer; i >= 0; --i) {
        ptr = block_read(bid)->data;
        bid = ptr[id[i]];
    }
    return bid;
}

// 遍历直接指针与间接指针
static u32_t next_block(ext2_inode_t *inode, u32_t bno) {
    if (file_size(inode) < (u64_t) bno * BLOCK_SIZE) {
        return 0;
    }
    if (bno < N_DIRECT_BLOCK)
        return inode->blocks[bno];

    return next_iBlock(inode, bno);
}

static u32_t nextFreeBlk(ext2_inode_t *inode) {
    u32_t bno;
    // bno 为需要分配的下一块的编号
    bno = inode->cntSectors / SECTOR_PER_BLOCK;
    if (bno < N_DIRECT_BLOCK) {
        return bno;
    }

    // 删除间接块指针本身占用的块
    bno -= 1;
    if (bno >= 12 + N_PTR) {
        bno -= 1 + N_PTR;
    };
    if (bno >= 12 + N_PTR + N_PTR * N_PTR) {
        bno -= 1 + N_PTR + N_PTR * N_PTR;
    }

    return bno;
}

static u32_t alloc_block(ext2_inode_t *inode, u32_t ino) {
    u32_t id[3], bno;
    int32_t layer;
    u32_t *ptr, *bid;
    buf_t *buf = NULL;

    bno = nextFreeBlk(inode);
    if (bno < N_DIRECT_BLOCK) {
        assertk(inode->blocks[bno] == 0);
        u32_t _bid = new_block(ino);
        inode->cntSectors += SECTOR_PER_BLOCK;
        inode->blocks[bno] = _bid;
        return _bid;
    }

    layer = find_block_layer(bno, id);

    bid = &inode->blocks[N_DIRECT_BLOCK + layer];
    for (int32_t i = layer; i >= -1; --i) {
        if (!(*bid)) {
            *bid = new_empty_block(ino);
            inode->cntSectors += SECTOR_PER_BLOCK;
            if (buf) block_write(buf);
            if (i == -1) break;
        }
        buf = block_read(*bid);
        ptr = buf->data;
        bid = &ptr[id[i]];
    }
    return *bid;
}

static void free_ptr(u32_t bid, u32_t ino, u32_t layer) {
    if (layer == 0) {
        free_block(ino, bid);
        return;
    };

    u32_t *ptr = block_read(bid)->data;
    for (u32_t i = 0; i < N_PTR && ptr[i]; ++i) {
        free_ptr(ptr[i], ino, layer - 1);
    }
    free_block(ino, bid);
}

// 删除 inode,释放数据块,不会修改 inode 所在目录内容
static void del_inode(ext2_inode_t *inode, u32_t ino) {
    assertk(ino > 0);
    u32_t bid;

    //释放间接指针
    for (int i = 0; i < N_DIRECT_BLOCK; ++i) {
        bid = inode->blocks[i];
        if (bid) free_block(ino, bid);
    }

    // 回收间接指针数据块
    for (int i = 0; i < 3; ++i) {
        bid = inode->blocks[N_DIRECT_BLOCK + i];
        if (bid) free_ptr(bid, ino, i + 1);
    }

    inode->delTime = cur_timestamp();
    inode->linkCnt = 0;
    free_inode(ino);
}


static u8_t get_clear_bit(u8_t ch) {
    u8_t no = 0;
    while (ch & 1) {
        ch >>= 1;
        no++;
    }
    return no;
}


static void free_block(u32_t ino, u32_t bno) {
    assertk(ino > 0);
    groupDesc_t *desc = get_descriptor(ino);
    buf_t *buf = block_read(desc->blockBitmapAddr);
    u8_t *map = buf->data;
    clear_bit(&map[bno / 8], bno % 8);
    block_write(buf);

    ext2_sb_t *blk = get_superBlock();
    blk->freeBlockCnt++;

    desc->freeBlockCnt++;
    write_descriptor(ino);
}

static void free_inode(u32_t ino) {
    assertk(ino > 0);
    groupDesc_t *desc = get_descriptor(ino);
    buf_t *buf = block_read(desc->inodeBitmapAddr);
    u8_t *map = buf->data;
    ino--;
    clear_bit(&map[ino / 8], ino % 8);
    block_write(buf);

    ext2_sb_t *blk = get_superBlock();
    blk->freeInodeCnt++;

    desc->freeInodesCnt++;
    write_descriptor(ino);
}


// 遍历位图找到可用 bit 并设置
static int32_t map_set_bit(u8_t *map, u32_t max) {
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


static void create_dir_entry(ext2_inode_t *inode, char *name, u32_t ino, u8_t type) {
    ext2_dir_t *dir, *new;
    u32_t bid;
    buf_t *buf;

    for_each_block(bid, inode) {
        buf = block_read(bid);
        dir = buf->data;
        for_each_dir(dir) {
            if (dir->inode == 0 && (new = new_dir_entry(dir, ino, name, type))) {
                merge_dir_entry(new);
                block_write(buf);
                return;
            }
        }
    }

    bid = alloc_block(inode, ino);
    update_inode_size(inode, BLOCK_SIZE);
    buf = block_read(bid);
    dir = buf->data;
    dir->inode = 0;
    dir->entrySize = BLOCK_SIZE;
    new_dir_entry(dir, ino, name, type);
    block_write(buf);
}

static ext2_dir_t *find_dir_entry(buf_t **buf, ext2_inode_t *parent, char *name) {
    size_t name_len = q_strlen(name);
    ext2_dir_t *dir;
    u32_t bid;
    buf_t *dBuf;

    for_each_block(bid, parent) {
        dBuf = block_read(bid);
        dir = dBuf->data;
        for_each_dir(dir) {
            if (dir->inode != 0 && dir->nameLen == name_len &&
                q_memcmp(name, dir->name, name_len)) {
                if (buf) *buf = dBuf;
                return dir;
            }
        }
    };
    return NULL;
}

static void del_dir_entry(ext2_dir_t *dir) {
    dir->inode = 0;
    merge_dir_entry(dir);
}


static ext2_sb_t *get_superBlock() {
    ext2_sb_t *blk;
    //TODO: 实际上应该检测设备和块号
    blk = block_read(0)->data + SECTOR_SIZE * SUPER_BLOCK_NO;
    assertk(blk->magic == EXT2_SIGNATURE);
    return blk;
}

static u32_t new_inode(u32_t ino) {
    assertk(ino > 0);
    ext2_sb_t *blk = get_superBlock();
    assertk(blk->freeInodeCnt > 0);
    for_each_group(ino) {
        groupDesc_t *desc = get_descriptor(ino);
        if (desc->freeInodesCnt == 0) continue;
        buf_t *buf = block_read(desc->inodeBitmapAddr);
        u8_t *map = buf->data;
        int32_t bit = map_set_bit(map, inodePerGroup / 8);
        block_write(buf);
        if (bit < 0) continue;
        blk->freeInodeCnt--;
        blk->writtenTime = cur_timestamp();
        desc->freeInodesCnt--;
        write_superBlock();
        write_descriptor(ino);
        // inode 号从 1 开始
        return bit + 1;
    }
    assertk(0);
    return 0;
}


static u32_t new_block(u32_t ino) {
    assertk(ino > 0);
    ext2_sb_t *blk = get_superBlock();
    assertk(blk->freeBlockCnt > 0);
    for_each_group(ino) {
        groupDesc_t *desc = get_descriptor(ino);
        if (desc->freeBlockCnt == 0) continue;
        buf_t *buf = block_read(desc->blockBitmapAddr);
        int32_t bit = map_set_bit(buf->data, BLOCK_PER_GROUP / 8);
        block_write(buf);
        if (bit >= 0) {
            blk->freeBlockCnt--;
            desc->freeBlockCnt--;
            write_descriptor(ino);
            write_superBlock();

            return bit;
        }
    }
    assertk(0);
    return 0;
}

static u32_t new_empty_block(u32_t ino) {
    // 分配新块,并将数据置 0
    u32_t bid = new_block(ino);
    buf_t *foo = block_read(bid);
    q_memset(foo->data, 0, BLOCK_SIZE);
    block_write(foo);
    return bid;
}


static void *get_descriptor(u32_t ino) {
    assertk(ino > 0);
    buf_t *buf = block_read(DESCRIPTOR_BID(ino));
    return buf->data + DESCRIPTOR_OFFSET(ino);
}

static void *get_inode(buf_t **buf, u32_t ino) {
    assertk(ino > 0);
    assertk(ino > 0 && ino < inodePerGroup * groupCnt);
    groupDesc_t *des = get_descriptor(ino);
    u32_t base = des->inodeTableAddr;
    u32_t blk_no = INODE_BID(base, ino);
    *buf = block_read(blk_no);
    return (void *) (*buf)->data + INODE_OFFSET(ino);
}

ext2_dir_t *next_entry(ext2_dir_t *dir) {
    return (void *) dir + dir->entrySize;
}

// 向后合并多个 inode 为 0 的目录项
static void merge_dir_entry(ext2_dir_t *dir) {
    assertk(dir);
    size_t size = 0;
    ext2_dir_t *hdr = dir;
    for_each_dir(hdr) {
        if (hdr->inode != 0)break;
        size += hdr->entrySize;
    }
    dir->entrySize = size;
}


static inode_t *inode2vnode(ext2_inode_t *inode, ext2_dir_t *dir) {
    inode_t *target = kmalloc(sizeof(inode_t));
    assertk(target);
    target->ino = dir->inode;
//    target->access_time = inode->accessTime;
//    target->modified_time = inode->modTime;
//    target->create_time = inode->createTime;
    target->size = inode->size;
    target->offset = 0;
    target->mode = inode->mode;
//    target->ref_cnt = 1;
//    q_memcpy(target->name, dir->name, dir->nameLen);
    return target;
}

static ext2_dir_t *new_dir_entry(ext2_dir_t *dir, u32_t ino, char *name, u8_t type) {
    assertk(dir->inode == 0);
    size_t name_len = q_strlen(name);
    u32_t size = ALIGN4(sizeof(ext2_dir_t) + name_len);

    if (size > dir->entrySize) return NULL;

    dir->inode = ino;
    dir->nameLen = name_len;
    dir->type = type;
    q_memcpy(dir->name, name, name_len);
    u32_t remain = dir->entrySize - size;
    if (remain > ALIGN4(sizeof(ext2_dir_t) + 1)) {
        dir->entrySize = size;
        dir = (void *) dir + size;
        dir->inode = 0;
        dir->entrySize = remain;
    }
    return dir;
}

static void write_superBlock() {
    buf_t *buf = page_get(0);
    block_write(buf);
}

static void write_descriptor(u32_t ino) {
    buf_t *buf = page_get(BLOCK2LBA(DESCRIPTOR_BID(ino)));
    block_write(buf);
}


//static void sb_backup() {
//    // 超级块备份同步
//
//}
//
//static void descriptor_backup(u32_t ino) {
//    // 块描述符同步
//}

#ifdef TEST
static inode_t root, foo, foo2, txt;
static char charBuf[4097] = "";

static void test() {
    buf_t *buf;
    ext2_dir_t *dir;
    root.ino = ROOT_INUM;
    ext2_inode_t *iRoot = get_inode(&buf, ROOT_INUM);
    //测试读
//    dir = find_dir_entry(&buf, iRoot, "txt");
//    txt.inode = dir->inode;
//    ext2_read(&txt, 0, 4097, charBuf);

    ext2_mkdir(&root, "foo");
    ext2_mkdir(&root, "foo2");
    ext2_mkdir(&root, "foo3");
//    ext2_rmdir(&root, "foo3");

    dir = find_dir_entry(&buf, iRoot, "foo");
    foo.ino = dir->inode;

    dir = find_dir_entry(&buf, iRoot, "foo2");
    foo2.ino = dir->inode;

    ext2_mkfile(&foo, "txt");

    //测试写
//    buf_t *fooBuf, *fooBuf2;
//    ext2_inode_t *inode = get_inode(&fooBuf, foo.inode);
//    dir = find_dir_entry(&fooBuf2, inode, "txt");
//    assertk(dir);
//    txt.inode = dir->inode;
//    ext2_write(&txt, 0, 4097, charBuf);

//    ext2_link(&foo, "txt", &foo2, "foo2");
//    ext2_link(&foo, "txt", &foo2, "foo3");
//    ext2_unlink(&foo2, "foo3");
    unblock_thread(flush_worker);
    ext2_ls(&root);
}

void test_ext2() {

}

#endif //TEST
