//
// Created by pjs on 2021/4/20.
//
// ext2 文件系统
// inode 号从 1 开始,块号从 0 开始
// 超级块与描述符表备份存储在块 BLOCK_PER_GROUP * i, i=1,3,5...
// DEBUG:  e2fsck -f disk.img
// TODO 将超级块与块描述符表同步到备份
// TODO: 权限管理,建立目录索引
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

static void del_node(inode_t *inode, u32_t ino);

static void *get_descriptor(u32_t ino);

static void *get_inode(buf_t **buf, u32_t ino);

static u32_t find_blk_id(inode_t *inode, u32_t offset);

static u32_t next_block(inode_t *inode, u32_t bno);

static void free_block(u32_t ino, u32_t bno);

static void free_inode(u32_t ino);

static u8_t get_clear_bit(u8_t ch);

static int32_t map_set_bit(u8_t *map, u32_t max);

static void create_dir_entry(inode_t *inode, char *name, u32_t ino, u8_t type);

static u32_t new_inode(u32_t ino);

static superBlock_t *get_superBlock();

static dir_t *next_entry(dir_t *dir);

static u32_t new_block(u32_t ino);

static void merge_dir_entry(dir_t *dir);

static dir_t *new_dir_entry(dir_t *dir, u32_t ino, char *name, u8_t type);

static dir_t *next_val_entry(dir_t *dir);

static void del_dir_entry(dir_t *dir);

static void write_descriptor(u32_t ino);

static u64_t file_size(inode_t *inode);

static void update_inode_size(inode_t *inode, int32_t change);

static void sb_backup();

static void descriptor_backup(u32_t ino);

static dir_t *find_dir_entry(buf_t **buf, inode_t *parent, char *name);

static void write_superBlock();
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

#define DIR_END(dir)            (((ptr_t)(dir) & BLOCK_SIZE) ? (ptr_t)(dir) + BLOCK_SIZE : \
                                            MEM_ALIGN((ptr_t)(dir),BLOCK_SIZE))

#define for_each_block(bid, inode)        u32_t bno = 0; while (((bid) = next_block(inode, bno++)))

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
    // 设置新 inode
    buf_t *buf;
    u32_t cur_time = cur_timestamp();
    u32_t ino = new_inode(parent->inode_num);
    inode_t *inode = get_inode(&buf, ino);
    q_memset(inode, 0, sizeof(inode_t));
    inode->createTime = cur_time;
    inode->accessTime = cur_time;
    inode->size = 0;
    inode->linkCnt = 3;
    inode->mode = EXT2_IFREG | EXT2_ALL_RWX;
    inode->linkCnt = 1;


    // 修改父目录 inode
    buf_t *p_buf;
    inode_t *p_inode = get_inode(&p_buf, parent->inode_num);
    create_dir_entry(p_inode, name, ino, EXT2_FT_REG_FILE);
    p_inode->accessTime = cur_time;
    p_inode->modTime = cur_time;
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

    if (inode->linkCnt == 0) {
        // 删除目录项
        del_dir_entry(target);
        del_node(inode, ino);
        cur_time = cur_timestamp();
        pInode->modTime = cur_time;
        pInode->linkCnt--;
        block_write(pBuf);
        block_write(tBuf);
    }

    block_write(buf);

    // 同步元数据
    groupDesc_t *desc = get_descriptor(target->inode);
    desc->dirNum--;
    write_descriptor(target->inode);

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


bool dir_empty(dir_t *dir) {
    dir_t *end = (void *) dir + BLOCK_SIZE;
    if (dir->inode != 0) {
        if (dir->nameLen != 1 || !q_memcmp(dir->name, ".", 1))
            return false;
        dir = next_entry(dir);
        if (dir > end)
            return true;
        if (dir->inode != 0 && (dir->nameLen != 2 || !q_memcmp(dir->name, "..", 2)))
            return false;
        dir = next_entry(dir);
    }
    for (; dir < end; dir = next_entry(dir)) {
        if (dir->inode != 0) return false;
    }
    return true;
}

void ext2_rmDir(fd_t *parent, char *name) {
    assertk(parent && name);
    u32_t cur_time;
    buf_t *pBuf, *iBuf, *tBuf;
    u32_t bno = 0, bid;

    // 查找
    inode_t *pInode = get_inode(&pBuf, parent->inode_num);
    dir_t *target = find_dir_entry(&tBuf, pInode, name);
    if (target == NULL) assertk(0);
    assertk(target->type == EXT2_FT_DIR);
    u32_t ino = target->inode;

    // 找到对应的 inode ,并判断目录是否为空
    inode_t *inode = get_inode(&iBuf, ino);
    while ((bid = next_block(inode, bno++))) {
        buf_t *buf2 = block_read(bid);
        if (!dir_empty(buf2->data)) assertk(0);
    }


    del_node(inode, ino);

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
    buf_t *pBuf1, *pBuf2, *tBuf1, *tBuf2;
    inode_t *p1Inode = get_inode(&pBuf1, parent->inode_num);
    dir_t *target = find_dir_entry(&tBuf1, p1Inode, target_name);
    if (!target || (target->type & EXT2_FT_DIR)) return;

    inode_t *p2Inode = get_inode(&pBuf2, parent2->inode_num);
    dir_t *foo = find_dir_entry(&tBuf2, p2Inode, new_name);
    if (foo) return;

    create_dir_entry(p2Inode, new_name, target->inode, target->type);
}

void ext2_chmod() {

}

// 打印 parent 目录内容
void ext2_ls(fd_t *parent) {
    buf_t *buf, *buf1;
    dir_t *dir, *end;
    inode_t *inode = get_inode(&buf, parent->inode_num);

    u32_t bno = 0, bid;

    while ((bid = next_block(inode, bno++))) {
        buf1 = block_read(bid);
        dir = (dir_t *) buf1->data;
        end = (void *) dir + BLOCK_SIZE;
        for (; dir < end; dir = next_entry(dir)) {
            prints((void *) dir->name, dir->nameLen);
            printfk("\n");
        }
    }
}

void ext2_open(fd_t *file) {
    buf_t *buf;
    inode_t *inode = get_inode(&buf, file->inode_num);
    inode->accessTime = cur_timestamp();
}


void ext2_read(fd_t *file, uint32_t _offset, uint32_t size, char *_buf) {
    buf_t *buf;
    inode_t *inode = get_inode(&buf, file->inode_num);
    u32_t offset = _offset % PAGE_SIZE; // 块内偏移
    u32_t bid = find_blk_id(inode, _offset);
    buf_t *buf1 = block_read(bid);
    q_memcpy(_buf, buf1->data + offset, size);

    u32_t cur_time = cur_timestamp();
    inode->accessTime = cur_time;
}

void ext2_write(fd_t *file, uint32_t _offset, uint32_t size, char *_buf) {
    buf_t *buf, *buf1;
    inode_t *inode = get_inode(&buf, file->inode_num);
    u32_t offset = _offset % PAGE_SIZE; // 块内偏移
    u32_t bid = find_blk_id(inode, _offset);
    buf1 = block_read(bid);
    q_memcpy(buf1->data + offset, _buf, size);

    u32_t cur_time = cur_timestamp();
    inode->accessTime = cur_time;
    inode->modTime = cur_time;
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
    assertk(bid != 0);
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

// 遍历直接指针与间接指针
static u32_t next_block(inode_t *inode, u32_t bno) {
    u32_t cnt = BLOCK_SIZE / sizeof(u32_t);
    u32_t *ptr, *ptr1, *ptr2;
    if (file_size(inode) < (u64_t) bno * BLOCK_SIZE) {
        return 0;
    }

    if (bno < N_DIRECT_BLOCK) {
        return inode->blocks[bno];
    }

    u32_t *bid = &inode->blocks[N_DIRECT_BLOCK];
    bno -= N_DIRECT_BLOCK;
    if (bno < cnt) {
        if (!(*bid)) return 0;
        ptr = block_read(*bid)->data;
        return ptr[bno];
    }

    bno -= cnt;
    if (bno < cnt * cnt) {
        bid += 1;
        if (!(*bid)) return 0;

        ptr = block_read(*bid)->data;
        ptr1 = block_read(ptr[bno / cnt])->data;
        return ptr1[bno % cnt];
    }

    bno -= cnt * cnt;
    if (bno < cnt * cnt * cnt) {
        bid += 2;
        if (!(*bid)) return 0;
        ptr = block_read(*bid)->data;
        ptr1 = block_read(ptr[bno / (cnt * cnt)])->data;
        ptr2 = block_read(ptr1[(bno / cnt) % cnt])->data;
        return ptr2[bno % cnt];
    }

    assertk(0);
    return 0;
}

static void free_siPtr(u32_t ino, u32_t *start, size_t size) {
    assertk(ino > 0);
    u32_t *end = start + size;
    for (; start < end && *start; start++) {
        free_block(ino, *start);
    }
}

static void free_diPtr(u32_t ino, u32_t *start) {
    assertk(ino > 0);
    u32_t *end = start + N_PTR;
    for (; start < end && *start; start++) {
        u32_t *tmp = block_read(*start)->data;
        free_siPtr(ino, tmp, N_PTR);
        free_block(ino, *tmp);
    }
}

// 删除 inode,释放数据块,不会修改 inode 所在目录内容
static void del_node(inode_t *inode, u32_t ino) {
    assertk(ino > 0);
    u32_t *ptr, *ptr1, *end;
    u32_t *bid;
    bid = &inode->blocks[0];

    // 回收直接指针
    free_siPtr(ino, bid, 12);

    bid += 12;
    // 回收间接指针数据块
    if (*bid) {
        ptr = block_read(*bid)->data;
        free_siPtr(ino, ptr, N_PTR);
        free_block(ino, *bid);
    }

    // 回收二重指针块
    bid++;
    if (*bid) {
        ptr = block_read(*bid)->data;
        free_diPtr(ino, ptr);
        free_block(ino, *bid);
    }

    // 回收三重指针块
    bid++;
    if (*bid) {
        ptr = block_read(*bid)->data;
        end = ptr + N_PTR;
        for (; ptr < end && *ptr; ptr++) {
            ptr1 = block_read(*ptr)->data;
            free_diPtr(ino, ptr1);
            free_block(ino, *ptr1);
        }
        free_block(ino, *bid);
    }
    inode->delTime = cur_timestamp();
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
    u8_t *map = block_read(desc->blockBitmapAddr)->data;
    desc->freeBlockCnt++;
    clear_bit(&map[bno / 8], bno % 8);

    superBlock_t *blk = get_superBlock();
    blk->freeBlockCnt++;
}

static void free_inode(u32_t ino) {
    assertk(ino > 0);
    groupDesc_t *desc = get_descriptor(ino);
    u8_t *map = block_read(desc->inodeBitmapAddr)->data;
    desc->freeInodesCnt++;
    ino--;
    clear_bit(&map[ino / 8], ino % 8);

    superBlock_t *blk = get_superBlock();
    blk->freeInodeCnt++;
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
    dir_t *dir, *end;
    u32_t bno = 0, bid;
    buf_t *buf;

    while (1) {
        bid = next_block(inode, bno++);
        if (bid == 0) {
            update_inode_size(inode, BLOCK_SIZE);
            bid = new_block(ino);
        }
        buf = block_read(bid);
        dir = buf->data;
        end = (void *) dir + BLOCK_SIZE;
        for (; dir < end; dir = next_entry(dir)) {
            if (dir->inode == 0 && new_dir_entry(dir, ino, name, type)) {
                merge_dir_entry(dir);
                block_write(buf);
                return;
            }
        }
    }
    assertk(0);
}

static dir_t *find_dir_entry(buf_t **buf, inode_t *parent, char *name) {
    size_t name_len = q_strlen(name);
    dir_t *dir, *end;
    u32_t bno = 0, bid;
    buf_t *_buf;

    while ((bid = next_block(parent, bno++))) {
        _buf = block_read(bid);
        dir = _buf->data;
        end = (void *) dir + BLOCK_SIZE;
        for (; dir < end; dir = next_entry(dir)) {
            if (dir->inode != 0 && dir->nameLen == name_len && q_memcmp(name, dir->name, name_len)) {
                if (buf) *buf = _buf;
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
    for (; ino <= groupCnt * inodePerGroup; ino += inodePerGroup) {
        groupDesc_t *desc = get_descriptor(ino);
        if (desc->freeInodesCnt != 0) {
            buf_t *buf = block_read(desc->inodeBitmapAddr);
            u8_t *map = buf->data;
            int32_t bit = map_set_bit(map, inodePerGroup / 8);
            block_write(buf);
            if (bit >= 0) {
                blk->freeInodeCnt--;
                blk->writtenTime = cur_timestamp();
                desc->freeInodesCnt--;
                write_superBlock();
                write_descriptor(ino);
                // inode 号从 1 开始
                return bit + 1;
            }
        }
    }
    assertk(0);
    return 0;
}


static u32_t new_block(u32_t ino) {
    assertk(ino > 0);
    superBlock_t *blk = get_superBlock();
    assertk(blk->freeBlockCnt > 0);
    for (; ino < groupCnt * inodePerGroup; ino += inodePerGroup) {
        groupDesc_t *desc = get_descriptor(ino);
        if (desc->freeBlockCnt != 0) {
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

static dir_t *next_val_entry(dir_t *dir) {
    return (void *) dir + ALIGN4(sizeof(dir_t) + dir->nameLen);
}

static dir_t *next_entry(dir_t *dir) {
    return (void *) dir + dir->entrySize;
}

// 向后合并多个 inode 为 0 的目录项
static void merge_dir_entry(dir_t *dir) {
    assertk(dir);
    size_t size = 0;
    dir_t *end = (void *) DIR_END(dir);
    dir_t *hdr = dir;
    for (; hdr < end && hdr->inode == 0; hdr = next_entry(hdr)) {
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
fd_t test;

void test_ext2_fs() {
    test.inode_num = ROOT_INUM;
//    ext2_rmDir(&test, "foo");
//    ext2_mkdir(&test, "foo");
    unblock_thread(flush_worker);
    ext2_ls(&test);
}

#endif //TEST