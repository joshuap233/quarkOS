//
// Created by pjs on 2021/4/20.
//
// ext2 文件系统
// inode 号从 1 开始,块号从 0 开始
// inode 位图与数据块位图占据一整个块
// 超级块与描述符表备份存储在块 BLOCK_PER_GROUP * i, i=1,3,5...
// TODO 将超级块与块描述符表同步到备份
// TODO: writtenTime
// TODO: 权限管理
#include "fs/ext2.h"
#include "fs/page_cache.h"
#include "types.h"
#include "lib/qlib.h"
#include "buf.h"
#include "fs/vfs.h"
#include "lib/qstring.h"
#include "drivers/cmos.h"
#include "mm/kmalloc.h"

// inode 的块号
#define INODE_INDEX(base, ino)  ((base)+((ino)-1)/INODE_PER_BLOCK)
// inode 在当前块中偏移
#define INODE_OFFSET(no)    ((((no)-1) % INODE_PER_BLOCK)*INODE_SIZE)

#define INODE2GROUP(ino) (((ino)-1)/inodePerGroup)

static u32_t groupCnt;
static u32_t inodePerGroup;


#define BLOCK2LBA(block_no) (BLOCK_SIZE / SECTOR_SIZE * (block_no))
// 目录项四字节对齐
#define ALIGN4(size) MEM_ALIGN(size, 4)

void read_super_block();

static void ext2_del_node(u32_t ino);

static void *get_descriptor(u32_t ino);

static void *get_inode(buf_t **buf, u32_t ino);

static u32_t find_blk_id(inode_t *inode, u32_t offset);

static u32_t *next_block(inode_t *inode, u32_t bno);

static void free_block(u32_t ino, u32_t bno);

static void free_inode(u32_t ino);

static u8_t get_clear_bit(u8_t ch);

static int32_t map_set_bit(u8_t *map, u32_t max);

static void create_dir_entry(inode_t *inode, char *name, u32_t ino, u8_t type);

static u32_t new_inode(u32_t ino);

static superBlock_t *get_superBlock();

static dir_t *next_entry(dir_t *dir);

static u32_t new_block(u32_t ino);

static dir_t *new_entry(dir_t *dir, u32_t ino, char *name, u8_t type);

static void del_dir_entry(fd_t *parent, char *name, u8_t type);

#define dir_len(dir) (sizeof(dir_t) + ALIGN4((dir)->nameLen))
#define block_read(blk_no)  page_read_no(BLOCK2LBA(blk_no))

#ifdef TEST

void test_ext2_mkdir();

#define block_write(buf) page_write_sync(buf)

#else

#define block_write(buf) page_write(buf)

#endif //TEST

void ext2_init() {
    superBlock_t *suBlock;
    suBlock = block_read(0)->data + SECTOR_SIZE * SUPER_BLOCK_NO;
    assertk(suBlock->magic == EXT2_SIGNATURE);
    assertk((2 << (suBlock->logBlockSize + 9)) == BLOCK_SIZE);
    assertk(suBlock->inodeSize == INODE_SIZE);

    groupCnt = suBlock->blockCnt / suBlock->blockPerGroup;
    assertk(suBlock->blockPerGroup == BLOCK_PER_GROUP);
    inodePerGroup = suBlock->inodePerGroup;
    if (suBlock->fsVersionMajor >= 1) {
        // required, optional feature
        // preallocate
    }

    buf_t *buf;
    groupDesc_t *descriptor = block_read(1)->data;
    inode_t *inode = get_inode(&buf, ROOT_INUM);
    assertk((inode->mode & MOD_DIR) == MOD_DIR);

    dir_t *dir = block_read(inode->blocks[0])->data;
    dir = (void *) dir + dir->entrySize;
    dir = (void *) dir + dir->entrySize;

    test_ext2_mkdir();
}

void read_super_block() {

}


void ext2_mkdir(fd_t *parent, char *name) {
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
    inode->size = BLOCK_SIZE;
    inode->linkCnt = 2;
    inode->mode = USER_MODE;
    block_write(buf);

    dir_t *dir = block_read(blk_no)->data;
    dir = new_entry(dir, ino, ".", DIR_DIR);
    new_entry(dir, parent->inode_num, "..", DIR_DIR);

    buf_t *buf1;
    inode_t *p_inode = get_inode(&buf1, parent->inode_num);
    create_dir_entry(p_inode, name, ino, DIR_DIR);
    // 修改 parent inode
    p_inode->accessTime = cur_time;
    p_inode->modTime = cur_time;
    p_inode->linkCnt++;
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
    inode->mode = USER_MODE;
    inode->linkCnt = 1;


    // 修改父目录 inode
    buf_t *p_buf;
    inode_t *p_inode = get_inode(&p_buf, parent->inode_num);
    create_dir_entry(p_inode, name, ino, DIR_REGULAR_FILE);
    p_inode->accessTime = cur_time;
    p_inode->modTime = cur_time;
}

void free_siPtr(u32_t ino, u32_t *start, size_t size) {
    u32_t *end = start + size;
    for (; start < end && *start; start++) {
        free_block(ino, *start);
    }
}

void free_diPtr(u32_t ino, u32_t *start) {
    u32_t *end = start + N_PTR;
    for (; start < end && *start; start++) {
        u32_t *tmp = block_read(*start)->data;
        free_siPtr(ino, tmp, N_PTR);
        free_block(ino, *tmp);
    }
}


// 文件操作,文件硬链接数为 1, 删除文件
void ext2_unlink(fd_t *parent, char *name) {
    del_dir_entry(parent, name, DIR_REGULAR_FILE);
}


fd_t *ext2_findDir(fd_t *parent, char *name) {
    buf_t *buf;
    size_t name_len = q_strlen(name);
    assertk(name_len + 1 < FILE_NAME_LEN);
    inode_t *p_inode = get_inode(&buf, parent->inode_num);
    dir_t *end, *dir;

    u32_t bno = 0, bid;
    while ((bid = *next_block(p_inode, bno++))) {
        dir = block_read(p_inode->blocks[bid])->data;
        end = (void *) dir + BLOCK_SIZE;
        for (; dir < end; dir = next_entry(dir)) {
            if (dir->nameLen == name_len && (dir->type & DIR_REGULAR_FILE)) {
                if (q_memcmp(name, dir->name, name_len)) {
                    fd_t *target = kmalloc(sizeof(fd_t));
                    q_memset(target, 0, sizeof(fd_t));
                    target->inode_num = dir->inode;
                    target->bid = bid;
                    q_memcpy(target->name, name, name_len);
                    target->name[name_len + 1] = '\0';
                    return target;
                }
            }
        }
    }
    return NULL;
}


bool dir_empty(dir_t *dir) {
    dir_t *end = (void *) dir + BLOCK_SIZE;
    assertk(dir->nameLen == 1 && q_memcmp(dir->name, ".", 1));
    dir = next_entry(dir);
    assertk(dir->nameLen == 2 && q_memcmp(dir->name, "..", 2))
    if (next_entry(dir) == end) {
        return true;
    }
    return false;
}

void ext2_rmDir(fd_t *parent, char *name) {
    buf_t *buf, *buf1, *buf2, *buf3;
    inode_t *p_inode = get_inode(&buf, parent->inode_num);
    fd_t *fd = ext2_findDir(parent, name);
    if (fd == NULL) {
        assertk(0);
    }

    inode_t *inode = get_inode(&buf1, fd->inode_num);
    assertk(inode->blocks[0]);
    buf2 = block_read(inode->blocks[0]);
    if (!inode->blocks[1]) {
        buf3 = block_read(inode->blocks[0]);
        if (dir_empty(buf3->data)) {
            del_dir_entry(parent, name, DIR_DIR);
        }
    }

    kfree(fd);
}

// 硬链接不能指向目录
void ext2_link(fd_t *parent) {

}

void ext2_chmod() {

}

// 打印 parent 目录内容
void ext2_ls(fd_t *parent) {
    buf_t *buf, *buf1;
    dir_t *dir, *end;
    inode_t *inode = get_inode(&buf, parent->inode_num);
    for (int i = 0; i < N_DIRECT_BLOCK; ++i) {
        if (inode->blocks[i]) {
            buf1 = block_read(inode->blocks[i]);
            dir = (dir_t *) buf1->data;
            end = (void *) dir + BLOCK_SIZE;
            for (; dir < end; dir = next_entry(dir)) {
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
    inode->modTime = cur_time;
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

void ext2_mount() {
    superBlock_t *blk = get_superBlock();
    blk->mountTime = cur_timestamp();
    blk->mountCnt++;
}

void ext2_close() {}


// 使用文件偏移找到直接指针
static u32_t find_blk_id(inode_t *inode, u32_t offset) {
    u32_t bid = *next_block(inode, offset / BLOCK_SIZE);
    assertk(bid != 0);
    return bid;
}


// 遍历直接指针与间接指针
static u32_t *next_block(inode_t *inode, u32_t bno) {
    u32_t cnt = BLOCK_SIZE / sizeof(u32_t);
    u32_t n_ptr = bno;
    u32_t *ptr, *ptr1, *ptr2;

    if (n_ptr < N_DIRECT_BLOCK) {
        assertk(inode->blocks[n_ptr]);
        return &inode->blocks[n_ptr];
    }
    u32_t *bid = &inode->blocks[N_DIRECT_BLOCK];
    n_ptr -= N_DIRECT_BLOCK;
    if (n_ptr < cnt) {
        assertk(*bid);
        ptr = block_read(*bid)->data;
        return &ptr[n_ptr];
    } else if (n_ptr < cnt * cnt) {
        bid += 1;
        assertk(*bid);
        n_ptr -= cnt;
        ptr = block_read(*bid)->data;
        ptr1 = block_read(ptr[n_ptr / cnt])->data;
        return &ptr1[n_ptr % cnt];
    } else if (n_ptr < cnt * cnt * cnt) {
        bid += 2;
        n_ptr -= cnt * cnt;
        assertk(*bid);
        ptr = block_read(*bid)->data;
        ptr1 = block_read(ptr[n_ptr / (cnt * cnt)])->data;
        ptr2 = block_read(ptr1[n_ptr % (cnt * cnt)])->data;
        return &ptr2[n_ptr % cnt];
    }

    assertk(0);
    return 0;
}

// 删除 inode,释放数据块,不会修改 inode 所在目录内容
static void ext2_del_node(u32_t ino) {
    buf_t *buf;
    inode_t *inode = get_inode(&buf, ino);
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

    bid++;
    if (*bid) {
        ptr = block_read(*bid)->data;
        free_diPtr(ino, ptr);
        free_block(ino, *bid);
    }

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
    free_inode(ino);
}


static u8_t get_clear_bit(u8_t ch) {
    u8_t no = 0;
    while (!(ch & 1)) {
        ch >>= 1;
        no++;
    }
    return no;
}


static void free_block(u32_t ino, u32_t bno) {
    groupDesc_t *desc = get_descriptor(ino);
    u8_t *map = block_read(desc->blockBitmapAddr)->data;
    clear_bit(&map[bno / 8], bno % 8);
}

static void free_inode(u32_t ino) {
    groupDesc_t *desc = get_descriptor(ino);
    u8_t *map = block_read(desc->blockBitmapAddr)->data;
    clear_bit(&map[ino / 8], ino % 8);
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
    u32_t blk_no;
    dir_t *dir;
    size_t name_len = q_strlen(name);

    for (int i = 0; i < N_DIRECT_BLOCK; ++i) {
        blk_no = inode->blocks[i];
        if (blk_no == 0) {
            inode->size += BLOCK_SIZE;
            blk_no = new_block(ino);
            inode->blocks[i] = blk_no;
        }
        dir = block_read(blk_no)->data;
        dir_t *end = (void *) dir + BLOCK_SIZE;
        for (; dir < end; dir = next_entry(dir)) {
            if (dir->entrySize - dir_len(dir) >= sizeof(dir_t) + name_len) {
                new_entry(dir, ino, name, type);
                break;
            }
        }
    }
}


// 如果需要删除的类型为常规文件,则 linkCnt--, linkCnt == 0 时删除文件
// 如果时目录则直接删除
static void del_dir_entry(fd_t *parent, char *name, u8_t type) {
    size_t name_len = q_strlen(name);
    dir_t *dir, *dir1, *start, *end;
    inode_t *inode, *p_inode;
    u32_t bno = 0, bid;
    u32_t cur_time;
    buf_t *buf, *buf1;

    p_inode = get_inode(&buf, parent->inode_num);
    while ((bid = *next_block(p_inode, bno++))) {
        start = block_read(bid)->data;
        end = (void *) start + BLOCK_SIZE;
        for (dir = start; dir < end; dir = next_entry(dir)) {
            if (dir->nameLen == name_len && q_memcmp(name, dir->name, name_len)) {
                cur_time = cur_timestamp();
                p_inode->accessTime = cur_time;
                p_inode->modTime = cur_time;
                // dir1 为需要删除的目录的前一项
                for (dir1 = start; next_entry(dir1) < dir; dir1 = next_entry(dir1));
                inode = get_inode(&buf1, dir->inode);
                assertk(type == dir->type);
                switch (type) {
                    case DIR_DIR:
                        ext2_del_node(dir->inode);
                        if (dir1 != dir) dir1->entrySize += dir->entrySize;
                        break;
                    case DIR_REGULAR_FILE:
                        inode->linkCnt--;
                        if (inode->linkCnt == 0) {
                            ext2_del_node(dir->inode);
                            if (dir1 != dir) dir1->entrySize += dir->entrySize;
                        }
                        break;
                    default: assertk(0);
                }
                return;
            }
        }
    };
    assertk(0);
}

static superBlock_t *get_superBlock() {
    superBlock_t *suBlock;
    suBlock = block_read(0)->data + SECTOR_SIZE * SUPER_BLOCK_NO;
    assertk(suBlock->magic == EXT2_SIGNATURE);
    return suBlock;
}

static void write_superBlock() {
    buf_t *buf = page_get(0);
    block_write(buf);
}

static void write_descriptor(u32_t ino) {
    u32_t group_no = INODE2GROUP(ino);
    buf_t *buf = page_get(1 + group_no / DESCRIPTOR_PER_BLOCK);
    block_write(buf);
}

static u32_t new_inode(u32_t ino) {
    superBlock_t *blk = get_superBlock();
    assertk(blk->freeInodeCnt > 0);
    for (; ino < groupCnt * inodePerGroup; ino += inodePerGroup) {
        groupDesc_t *desc = get_descriptor(ino);
        if (desc->freeBlockCnt != 0) {
            buf_t *buf = block_read(desc->blockBitmapAddr);
            u8_t *map = buf->data;
            int32_t bit = map_set_bit(map, DIV_CEIL(inodePerGroup, 8));
            block_write(buf);
            if (bit >= 0) {
                blk->freeInodeCnt--;
                desc->freeInodesCnt--;
                write_superBlock();
                write_descriptor(ino);
                return bit;
            }
        }
    }
}


static u32_t new_block(u32_t ino) {
    superBlock_t *blk = get_superBlock();
    assertk(blk->freeBlockCnt > 0);
    for (; ino < groupCnt * inodePerGroup; ino += inodePerGroup) {
        groupDesc_t *desc = get_descriptor(ino);
        assertk(desc->freeBlockCnt != 0);
        buf_t *buf = block_read(desc->blockBitmapAddr);
        u8_t *map = buf->data;
        int32_t bit = map_set_bit(map, BLOCK_SIZE / 8);
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

static void *get_descriptor(u32_t ino) {
    u32_t group_no = INODE2GROUP(ino);
    buf_t *buf = block_read(1 + group_no / DESCRIPTOR_PER_BLOCK);
    return buf->data + group_no % DESCRIPTOR_PER_BLOCK;
}

static void *get_inode(buf_t **buf, u32_t ino) {
    assertk(ino > 0 && ino < inodePerGroup * groupCnt);
    groupDesc_t *des = get_descriptor(ino);
    u32_t base = des->inodeTableAddr;
    u32_t blk_no = INODE_INDEX(base, ino);
    *buf = block_read(blk_no);
    return (void *) (*buf)->data + INODE_OFFSET(ino);
}

static dir_t *next_entry(dir_t *dir) {
    return (void *) dir + dir->entrySize;
}

// 在 dir 目录项后创建新目录
static dir_t *new_entry(dir_t *dir, u32_t ino, char *name, u8_t type) {
    size_t name_len = q_strlen(name);
    u16_t dirLen = dir_len(dir);
    u16_t remain = dir->entrySize - dirLen;
    dir = (void *) dir + dirLen;
    dir->inode = ino;
    dir->nameLen = name_len;
    dir->entrySize = remain;
    dir->type = type;
    q_memcpy(dir->name, name, name_len);
    return dir;
}


#ifdef TEST
fd_t test;

void test_ext2_mkdir() {
    test.inode_num = ROOT_INUM;
    ext2_mkdir(&test, "foo");
}

#endif //TEST