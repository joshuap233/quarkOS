//
// Created by pjs on 2021/4/20.
//
// ext2 文件系统
// inode 号从 1 开始,块号从 0 开始
// 超级块与描述符表备份存储在块 BLOCK_PER_GROUP * i, i=1,3,5...
// DEBUG:  e2fsck -f disk.img
// TODO 将超级块与块描述符表同步到备份
// TODO: 权限管理,建立目录索引
// TODO: 使用 inode,desc ... 查找 buf(写个类似 buf_entry 的就行,毕竟页缓存页对齐)
// 稀疏块: 不分配 inode->block 但设置 inode->size
#include "fs/ext2.h"
#include "fs/page_cache.h"
#include "types.h"
#include "lib/qlib.h"
#include "buf.h"
#include "fs/vfs.h"
#include "lib/qstring.h"
#include "drivers/cmos.h"
#include "mm/kmalloc.h"
#include "sched/kthread.h"

/*---  目录项操作 ---*/
dir_t *next_entry(dir_t *dir);

INLINE bool dir_empty(inode_t *inode);

static dir_t *new_dir_entry(dir_t *dir, u32_t ino, char *name, u8_t type);

static dir_t *find_dir_entry(buf_t **buf, inode_t *parent, char *name);

static void create_dir_entry(inode_t *inode, char *name, u32_t ino, u8_t type);

static void del_dir_entry(dir_t *dir);

static void merge_dir_entry(dir_t *dir);

/*--- 数据块操作 ---*/

static u32_t find_blk_id(inode_t *inode, u32_t offset);

static u32_t next_block(inode_t *inode, u32_t bno);

static void free_block(u32_t ino, u32_t bno);

static u32_t new_block(u32_t ino);

static u32_t alloc_block(inode_t *inode, u32_t ino);

/*--- inode 操作 ---*/
static u32_t new_inode(u32_t ino);

static void *get_inode(buf_t **buf, u32_t ino);

static void free_inode(u32_t ino);

static void del_inode(inode_t *inode, u32_t ino);

static void update_inode_size(inode_t *inode, int32_t change);

/*--- 组与超级块数据操作  ---*/

static void sb_backup();

static void write_superBlock();

static superBlock_t *get_superBlock();

static void *get_descriptor(u32_t ino);

static void descriptor_backup(u32_t ino);

static void write_descriptor(u32_t ino);

/*--- 其他  ---*/
static u64_t file_size(inode_t *inode);

static u8_t get_clear_bit(u8_t ch);

static int32_t map_set_bit(u8_t *map, u32_t max);

// inode 的块号
#define INODE_BID(base, ino)    ((base)+((ino)-1)%inodePerGroup / INODE_PER_BLOCK)
// inode 在当前块中偏移
#define INODE_OFFSET(no)        ((((no)-1) % INODE_PER_BLOCK) * INODE_SIZE)

#define INODE2GROUP(ino)        (((ino)-1) / inodePerGroup)

#define DESCRIPTOR_OFFSET(ino)  ((((ino)-1) / inodePerGroup) * sizeof(groupDesc_t))

#define DESCRIPTOR_BID(ino)     (1 + INODE2GROUP(ino) / DESCRIPTOR_PER_BLOCK)

#define BLOCK2LBA(block_no)     (BLOCK_SIZE / SECTOR_SIZE * (block_no))
// 目录项四字节对齐
#define ALIGN4(size)            MEM_ALIGN(size, 4)

#define dir_len(dir)            (ALIGN4(sizeof(dir_t) + (dir)->nameLen))

#define block_read(blk_no)      page_read_no(BLOCK2LBA(blk_no))

#define block_write(buf)        page_write(buf)

#define DIR_END(dir)            (((dir) & (~(BLOCK_SIZE-1))) + BLOCK_SIZE)

#define for_each_block(bid, inode)        for(u32_t bno=0;((bid) = next_block(inode, bno++));)

#define for_each_dir(dir)                 for (dir_t* end = (void *) DIR_END((ptr_t)(dir)); (dir) < end; (dir) = next_entry(dir))

#define for_each_group(ino)               for (; (ino) <= groupCnt * inodePerGroup; (ino) += inodePerGroup)


static u32_t groupCnt;
static u32_t inodePerGroup;
static u32_t version;

void ext2_init() {
    superBlock_t *blk;
    blk = block_read(0)->data + SECTOR_SIZE * SUPER_BLOCK_NO;
    assertk(blk->magic == EXT2_SIGNATURE);
    assertk((2 << (blk->logBlockSize + 9)) == BLOCK_SIZE);
    assertk(blk->inodeSize == INODE_SIZE);
    groupCnt = blk->blockCnt / blk->blockPerGroup;
    assertk(blk->blockPerGroup == BLOCK_PER_GROUP);
    inodePerGroup = blk->inodePerGroup;
    assertk(inodePerGroup <= BLOCK_SIZE * 8);
    version = blk->fsVersionMajor;
    if (blk->fsVersionMajor >= 1) {
        // required, optional feature
        // preallocate
    }

    buf_t *buf, *buf2;
    groupDesc_t *descriptor = get_descriptor(ROOT_INUM);
    inode_t *inode = get_inode(&buf, ROOT_INUM);
    assertk(inode->mode & EXT2_IFDIR);

    buf2 = block_read(inode->blocks[0]);
    dir_t *dir = buf2->data;
    dir_t *end = (void *) dir + BLOCK_SIZE;

    for (; next_entry(dir) < end; dir = next_entry(dir));
    if (dir->inode == 0)return;
    u32_t len = dir_len(dir);
    u32_t remain = dir->entrySize - len;
    if (remain > ALIGN4(sizeof(dir_t) + len)) {
        dir->entrySize = len;
        dir = next_entry(dir);
        dir->inode = 0;
        dir->entrySize = remain;
        block_write(buf2);
    }
}


void ext2_mkdir(fd_t *parent, char *name) {
    buf_t *pBuf;
    inode_t *pInode = get_inode(&pBuf, parent->inode_num);
    dir_t *target = find_dir_entry(NULL, pInode, name);
    if (target)return;

    // 设置新 inode 与目录项
    buf_t *buf;
    u32_t cur_time = cur_timestamp();
    u32_t ino = new_inode(parent->inode_num);
    u32_t blk_no = new_block(parent->inode_num);
    inode_t *inode = get_inode(&buf, ino);
    q_memset(inode, 0, sizeof(inode_t));
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
    dir_t *dir = buf1->data;
    dir->inode = 0;
    dir->entrySize = BLOCK_SIZE;
    dir = new_dir_entry(dir, ino, ".", EXT2_FT_DIR);
    new_dir_entry(dir, parent->inode_num, "..", EXT2_FT_DIR);
    block_write(buf1);


    create_dir_entry(pInode, name, ino, EXT2_FT_DIR);
    // 修改 parent inode
    pInode->accessTime = cur_time;
    pInode->modTime = cur_time;
    pInode->linkCnt++;
    block_write(pBuf);
}


void ext2_mkfile(fd_t *parent, char *name) {
    buf_t *pBuf;
    inode_t *pInode = get_inode(&pBuf, parent->inode_num);
    dir_t *target = find_dir_entry(NULL, pInode, name);
    if (target)return;

    // 设置新 inode 与目录项
    buf_t *buf;
    u32_t cur_time = cur_timestamp();
    u32_t ino = new_inode(parent->inode_num);
    inode_t *inode = get_inode(&buf, ino);
    q_memset(inode, 0, sizeof(inode_t));
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
void ext2_unlink(fd_t *parent, char *name) {
    assertk(parent && name);
    u32_t cur_time;
    buf_t *pBuf, *buf, *tBuf;

    // 查找
    inode_t *pInode = get_inode(&pBuf, parent->inode_num);
    dir_t *target = find_dir_entry(&tBuf, pInode, name);
    if (target == NULL) assertk(0);
    u32_t ino = target->inode;

    assertk(target->type = EXT2_FT_REG_FILE);

    // 找到对应的 inode
    inode_t *inode = get_inode(&buf, ino);
    inode->linkCnt--;
    assertk(inode->linkCnt >= 0);

    del_dir_entry(target);
    if (inode->linkCnt == 0) {
        del_inode(inode, ino);
        cur_time = cur_timestamp();
        pInode->modTime = cur_time;
        block_write(pBuf);
        block_write(tBuf);
    }

    block_write(buf);

    // 同步元数据
    superBlock_t *blk = get_superBlock();
    blk->writtenTime = cur_time;
    write_superBlock();
}


fd_t *ext2_find(fd_t *parent, char *name, u16_t type) {
    buf_t *buf, *tBuf;
    size_t name_len = q_strlen(name);
    assertk(name_len + 1 < FILE_NAME_LEN);

    inode_t *inode = get_inode(&buf, parent->inode_num);
    dir_t *dir = find_dir_entry(&tBuf, inode, name);
    if (!dir) return NULL;

    fd_t *target = kcalloc(sizeof(fd_t));
    target->inode_num = dir->inode;
    q_memcpy(target->name, name, name_len);
    target->name[name_len + 1] = '\0';
    target->type = type;
    return target;
}


INLINE bool dir_empty(inode_t *inode) {
    assertk(inode->mode & EXT2_IFDIR);
    return inode->linkCnt == 2;
}

void ext2_rmDir(fd_t *parent, char *name) {
    assertk(parent && name);
    u32_t cur_time;
    buf_t *pBuf, *iBuf, *tBuf;

    // 查找
    inode_t *pInode = get_inode(&pBuf, parent->inode_num);
    dir_t *target = find_dir_entry(&tBuf, pInode, name);
    if (target == NULL) assertk(0);
    assertk(target->type == EXT2_FT_DIR);
    u32_t ino = target->inode;

    // 找到对应的 inode ,并判断目录是否为空
    inode_t *inode = get_inode(&iBuf, ino);
    assertk(dir_empty(inode));

    del_inode(inode, ino);

    // 删除目录项
    del_dir_entry(target);
    block_write(tBuf);
    block_write(iBuf);

    cur_time = cur_timestamp();
    pInode->accessTime = cur_time;
    pInode->modTime = cur_time;
    pInode->linkCnt--;
    block_write(pBuf);

    // 同步元数据
    groupDesc_t *desc = get_descriptor(ino);
    desc->dirNum--;
    write_descriptor(ino);

    superBlock_t *blk = get_superBlock();
    blk->writtenTime = cur_time;
    write_superBlock();
}

// 硬链接不能指向目录
void ext2_link(fd_t *parent, char *target_name, fd_t *parent2, char *new_name) {
    buf_t *pBuf1, *pBuf2, *tBuf1, *tBuf2, *tgBuf;
    inode_t *p1Inode = get_inode(&pBuf1, parent->inode_num);
    dir_t *target = find_dir_entry(&tBuf1, p1Inode, target_name);
    if (!target || (target->type & EXT2_FT_DIR)) return;

    inode_t *p2Inode = get_inode(&pBuf2, parent2->inode_num);
    dir_t *foo = find_dir_entry(&tBuf2, p2Inode, new_name);
    if (foo) return;

    u32_t cur_time = cur_timestamp();

    inode_t *tin = get_inode(&tgBuf, target->inode);
    tin->linkCnt++;
    tin->modTime = cur_time;

    p1Inode->accessTime = cur_time;
    p2Inode->accessTime = cur_time;
    p2Inode->modTime = cur_time;

    create_dir_entry(p2Inode, new_name, target->inode, target->type);
    block_write(pBuf1);
    block_write(pBuf2);
    block_write(tgBuf);
}

void ext2_chmod() {

}

// 打印 parent 目录内容
void ext2_ls(fd_t *parent) {
    buf_t *buf, *buf1;
    dir_t *dir;
    inode_t *inode = get_inode(&buf, parent->inode_num);

    u32_t bid;

    for_each_block(bid, inode) {
        buf1 = block_read(bid);
        dir = (dir_t *) buf1->data;
        for_each_dir(dir) {
            if (dir->inode != 0) {
                prints((void *) dir->name, dir->nameLen);
                printfk("\n");
            }
        }
    }
}

void ext2_open(fd_t *file) {
    buf_t *buf;
    inode_t *inode = get_inode(&buf, file->inode_num);
    inode->accessTime = cur_timestamp();
    block_write(buf);
}


u32_t ext2_read(fd_t *file, uint32_t offset, uint32_t size, char *buf) {
    buf_t *iBuf, *dBuf;
    inode_t *inode;
    u32_t rdSize, blkOffset, bid;
    u64_t end;

    inode = get_inode(&iBuf, file->inode_num);
    end = file_size(inode);
    if (offset > end) return 0;

    if (offset + size > end)
        size = end - offset;

    while (size > 0) {
        rdSize = size > BLOCK_SIZE ? BLOCK_SIZE : size;
        blkOffset = offset % PAGE_SIZE;      // 块内偏移
        bid = find_blk_id(inode, offset);
        assertk(bid);

        dBuf = block_read(bid);
        q_memcpy(buf, dBuf->data + blkOffset, rdSize);

        size -= rdSize;
        buf += BLOCK_SIZE;
        offset += rdSize;
    }

    u32_t cur_time = cur_timestamp();
    inode->accessTime = cur_time;
    block_write(iBuf);
    return size;
}

u32_t ext2_write(fd_t *file, uint32_t offset, uint32_t size, char *buf) {
    //TODO:
    buf_t *iBuf, *dBuf;
    inode_t *inode;
    u32_t rdSize, blkOffset, bid;
    u64_t end;

    inode = get_inode(&iBuf, file->inode_num);
    end = file_size(inode);
    if (offset > end) return 0;

    if (offset + size > end)
        size = end - offset;

    while (size > 0) {
        rdSize = size > BLOCK_SIZE ? BLOCK_SIZE : size;
        blkOffset = offset % PAGE_SIZE;      // 块内偏移
        bid = find_blk_id(inode, offset);
        assertk(bid);

        dBuf = block_read(bid);
        q_memcpy(dBuf->data + blkOffset, buf, rdSize);

        size -= rdSize;
        buf += BLOCK_SIZE;
        offset += rdSize;
    }

    u32_t cur_time = cur_timestamp();
    inode->accessTime = cur_time;
    inode->modTime = cur_time;
    block_write(iBuf);
}

void ext2_umount() {
    buf_t *rBuf, *tBuf;
    inode_t *root = get_inode(&rBuf, ROOT_INUM);
    dir_t *target = find_dir_entry(&tBuf, root, "..");
    target->inode = ROOT_INUM;
    block_write(tBuf);
}

void ext2_mount(fd_t *parent) {
    buf_t *rBuf, *tBuf;
    inode_t *root = get_inode(&rBuf, ROOT_INUM);
    dir_t *target = find_dir_entry(&tBuf, root, "..");
    target->inode = parent->inode_num;
    block_write(tBuf);

    superBlock_t *blk = get_superBlock();
    blk->mountTime = cur_timestamp();
    blk->mountCnt++;
    write_superBlock();
}

void ext2_close() {}


// 使用文件偏移找到直接指针
static u32_t find_blk_id(inode_t *inode, u32_t offset) {
    u32_t bid = next_block(inode, offset / BLOCK_SIZE);
    return bid;
}

static void update_inode_size(inode_t *inode, int32_t change) {
    u64_t size = file_size(inode);
    if (change < 0) assertk(size >= BLOCK_SIZE);
    size += change;
    if (version >= 1 && (inode->mode & EXT2_IFREG)) {
        inode->dirAcl = size >> 32;
    }
    inode->size = size & MASK_U64(32);
}

static u64_t file_size(inode_t *inode) {
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

// 遍历直接指针与间接指针
static u32_t next_block(inode_t *inode, u32_t bno) {
    u32_t *ptr;
    u32_t bid;
    int32_t layer;
    u32_t id[3];
    if (file_size(inode) < (u64_t) bno * BLOCK_SIZE) {
        return 0;
    }
    if (bno < N_DIRECT_BLOCK)
        return inode->blocks[bno];

    layer = find_block_layer(bno, id);

    bid = inode->blocks[N_DIRECT_BLOCK + layer];
    for (int32_t i = layer; i >= 0; --i) {
        ptr = block_read(bid)->data;
        bid = ptr[id[i]];
    }
    return bid;
}


static u32_t alloc_block(inode_t *inode, u32_t ino) {
    u32_t id[3],bno;
    int32_t layer;

    bno = inode->cntSectors / SECTOR_PER_BLOCK;
    if (bno < N_DIRECT_BLOCK) {
        assertk(inode->blocks[bno] == 0);
        u32_t bid = new_block(ino);
        inode->blocks[bno] = bid;
        return bid;
    }
    // 删除间接块指针本身占用的块
    bno -= 1;
    if (bno >= 12 + N_PTR) {
        bno -= 1 + N_PTR;
    };
    if (bno >= 12 + N_PTR + N_PTR * N_PTR) {
        bno -= 1 + N_PTR + N_PTR * N_PTR;
    }

    layer = find_block_layer(bno, id);

    u32_t *ptr,*bid;
    buf_t *buf = NULL;
    bid = &inode->blocks[N_DIRECT_BLOCK + layer];
    for (int32_t i = layer; i >= 0; --i) {
        if (!(*bid)) {
            *bid = new_block(ino);
            inode->cntSectors++;
            if (buf) block_write(buf);

            buf = block_read(*bid);
            ptr = buf->data;
            ptr[id[i]] = 0;
        } else {
            buf = block_read(*bid);
            ptr = buf->data;
        }
        bid = &ptr[id[i]];
    }

    inode->cntSectors++;
    *bid = new_block(ino);
    block_write(buf);
    return *bid;
}

static void free_ptr(u32_t bid, u32_t ino, u32_t layer) {
    if (layer == 0) {
        free_block(ino, bid);
        return;
    };

    u32_t *ptr = block_read(bid)->data;
    for (u32_t i = 0; i < N_PTR; ++i) {
        if (ptr[i]) free_ptr(ptr[i], ino, layer - 1);
    }
    free_block(ino, bid);
}

// 删除 inode,释放数据块,不会修改 inode 所在目录内容
static void del_inode(inode_t *inode, u32_t ino) {
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

    superBlock_t *blk = get_superBlock();
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

    superBlock_t *blk = get_superBlock();
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


static void create_dir_entry(inode_t *inode, char *name, u32_t ino, u8_t type) {
    dir_t *dir, *new;
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

    // TODO:bid 添加到 inode->block
    bid = new_block(ino);
    update_inode_size(inode, BLOCK_SIZE);
    buf = block_read(bid);
    dir = buf->data;
    dir->inode = 0;
    dir->entrySize = BLOCK_SIZE;
    new_dir_entry(dir, ino, name, type);
    block_write(buf);
}

static dir_t *find_dir_entry(buf_t **buf, inode_t *parent, char *name) {
    size_t name_len = q_strlen(name);
    dir_t *dir;
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

static void del_dir_entry(dir_t *dir) {
    dir->inode = 0;
    merge_dir_entry(dir);
}


static superBlock_t *get_superBlock() {
    superBlock_t *blk;
    blk = block_read(0)->data + SECTOR_SIZE * SUPER_BLOCK_NO;
    assertk(blk->magic == EXT2_SIGNATURE);
    return blk;
}

static u32_t new_inode(u32_t ino) {
    assertk(ino > 0);
    superBlock_t *blk = get_superBlock();
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
    superBlock_t *blk = get_superBlock();
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

dir_t *next_entry(dir_t *dir) {
    return (void *) dir + dir->entrySize;
}

// 向后合并多个 inode 为 0 的目录项
static void merge_dir_entry(dir_t *dir) {
    assertk(dir);
    size_t size = 0;
    dir_t *hdr = dir;
    for_each_dir(hdr) {
        if (hdr->inode != 0)break;
        size += hdr->entrySize;
    }
    dir->entrySize = size;
}

static dir_t *new_dir_entry(dir_t *dir, u32_t ino, char *name, u8_t type) {
    assertk(dir->inode == 0);
    size_t name_len = q_strlen(name);
    u32_t size = ALIGN4(sizeof(dir_t) + name_len);

    if (size > dir->entrySize) return NULL;

    dir->inode = ino;
    dir->nameLen = name_len;
    dir->type = type;
    q_memcpy(dir->name, name, name_len);
    u32_t remain = dir->entrySize - size;
    if (remain > ALIGN4(sizeof(dir_t) + 1)) {
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


static void sb_backup() {
    // 超级块备份同步

}

static void descriptor_backup(u32_t ino) {
    // 块描述符同步
}

#ifdef TEST
static fd_t root, foo, foo2, foo3;

static void test_ext2_fs();

void test_ext2() {
    root.inode_num = ROOT_INUM;
    buf_t *buf, *buf1;
    inode_t *inode = get_inode(&buf, ROOT_INUM);
    dir_t *dir = block_read(inode->blocks[0])->data;
    dir = next_entry(dir);
    dir = next_entry(dir);
    dir = next_entry(dir);

    inode_t *inode1 = get_inode(&buf1, dir->inode);
    alloc_block(inode1, dir->inode);
}

void test_ext2_fs() {
    root.inode_num = ROOT_INUM;

    ext2_mkdir(&root, "foo");
    ext2_mkdir(&root, "foo2");
    ext2_mkdir(&root, "foo3");
    ext2_rmDir(&root, "foo3");

    foo.inode_num = 12;
    foo2.inode_num = 13;
    ext2_mkfile(&foo, "txt");
    ext2_link(&foo, "txt", &foo2, "foo2");
    ext2_link(&foo, "txt", &foo2, "foo3");
    ext2_unlink(&foo2, "foo3");
    unblock_thread(flush_worker);
    ext2_ls(&root);
}

#endif //TEST