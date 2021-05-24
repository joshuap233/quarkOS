//
// Created by pjs on 2021/4/20.
//
// ext2 文件系统
// 使用页缓存磁盘原始数据, inoCache,dirCache 缓存 vfs
// 定时 同步 inoCache,dirCache 到页缓存

/*
 * 备忘:
 * inode 号从 1 开始,块号从 0 开始
 * 超级块与描述符表备份存储在块 BLOCK_PER_GROUP * i, i=1,3,5...
 * DEBUG:  e2fsck -f disk.img
 * TODO: 权限管理,建立目录索引
 */
#include <fs/ext2/ext2.h>
#include <fs/page_cache.h>
#include <lib/qlib.h>
#include <fs/buf.h>
#include <fs/vfs.h>
#include <lib/qstring.h>
#include <mm/kmalloc.h>
#include <mm/slab.h>
#include <drivers/cmos.h>
#include <fs/ext2/block.h>


#define ALIGN4(size)            MEM_ALIGN(size, 4)

#define dir_len(dir)            (ALIGN4(sizeof(ext2_dir_t) + (dir)->nameLen))

#define DIR_END(dir)            (((dir) & (~(BLOCK_SIZE-1))) + BLOCK_SIZE)

#define for_each_dir(dir)                 for (ext2_dir_t* end = (void *) DIR_END((ptr_t)(dir)); (dir) < end; (dir) = next_entry(dir))


static void ops_init();

static directory_t *directory_cpy(ext2_dir_t *src, u32_t ino);

static struct fs_ops ops;
static list_head_t inode_lru;
static list_head_t inode_dirty;
static struct slabCache inoCache;
static struct slabCache dirCache;

ext2_dir_t *next_entry(ext2_dir_t *dir) {
    return (void *) dir + dir->entrySize;
}

void check_superBlock(super_block_t *_sb) {
    ext2_sb_info_t *sb = ext2_s(_sb);
    assertk(sb->sb.magic == EXT2_SIGNATURE);
    assertk(sb->sb.blockSize == BLOCK_SIZE);
    assertk(sb->sb.inodeSize == INODE_SIZE);
    assertk(sb->blockPerGroup == BLOCK_PER_GROUP);
    assertk(sb->inodePerGroup <= BLOCK_SIZE * 8);
}

static ext2_inode_t *get_raw_inode(super_block_t *_sb, u32_t ino) {
    ext2_sb_info_t *sb = ext2_s(_sb);
    u32_t base = sb->desc[EXT2_INODE2GROUP(ino, sb)].inodeTableAddr;
    u32_t blk_no = (base) + ((ino) - 1) % sb->inodePerGroup / INODE_PER_BLOCK;
    buf_t *buf = ext2_block_read(blk_no)->data;
    return (void *) buf + ((ino - 1) % sb->inodePerGroup) * sb->sb.inodeSize;
}


// 打开一个目录后,将他的子目录全部缓存到 dir->child
// 打开文件,则预读一块
void ext2_open(inode_t *inode) {
    list_head_t *hdr;
    ext2_dir_t *dir;
    directory_t *vfs_dir, *parent = inode->dir;
    ext2_inode_info_t *info = ext2_i(inode);

    inode->refCnt++;
    if (!list_empty(&inode->page)) {
        return;
    }
    if (inode->mode & EXT2_IFDIR) {
        map_block(inode, ext2_i(inode->sb)->blocks[0], 0);
        return;
    }

    map_all_blocks(info);


    list_for_each(hdr, &info->inode.page) {
        dir = page_entry(hdr)->data;
        if (dir->nameLen == 1 && q_strcmp((char *) dir->name, "."))
            dir = next_entry(dir);

        for_each_dir(dir) {
            vfs_dir = directory_cpy(dir, dir->inode);
            vfs_dir->parent = parent;
            if (!parent->child) {
                parent->child = vfs_dir;
            }
            list_add_prev(&vfs_dir->list, &parent->child->list);
        }
    }
    list_for_each(hdr, &inode->page) {
        page_entry(hdr)->ref_cnt--;
    }
    list_header_init(&info->inode.page);
}

inode_t *inode_cpy(u32_t ino, super_block_t *sb) {
    // 拷贝磁盘原始数据到 vfs 缓存
    ext2_inode_t *ei = get_raw_inode(sb, ino);
    ext2_inode_info_t *info = cache_alloc(&inoCache);
    inode_t *inode = &info->inode;

    inode->refCnt = 0;
    inode->mode = ei->mode;
    inode->type = 0;
    inode->linkCnt = ei->linkCnt;
    inode->size = ei->size;
    inode->pageCnt = ei->cntSectors * 8;
    inode->createTime = ei->createTime;
    inode->accessTime = ei->accessTime;
    inode->modifiedTime = ei->modTime;
    inode->deleteTime = ei->delTime;
    inode->offset = 0;
    inode->sb = sb;
    inode->ops = &ops;
    list_header_init(&inode->list.lru);
    list_header_init(&inode->page);
    rwlock_init(&inode->rwlock);
    q_memcpy(info->blocks, ei->blocks, sizeof(u32_t) * N_BLOCKS);

    return inode;
}

directory_t *alloc_directory(const char *name, u32_t ino) {
    u32_t len = q_strlen(name);
    directory_t *dir = cache_alloc(&dirCache);
    dir->dirty = false;
    dir->ino = ino;
    dir->inode = NULL;
    q_memcpy(dir->name, name, len);
    dir->name[dir->nameLen] = '\0';
    dir->nameLen = len + 1;

    dir->parent = NULL;
    dir->child = NULL;
    list_header_init(&dir->list);
    list_header_init(&dir->link);
    rwlock_init(&dir->rwLock);
    return dir;
}

directory_t *directory_cpy(ext2_dir_t *src, u32_t ino) {
    directory_t *dir = cache_alloc(&dirCache);
    dir->dirty = false;
    dir->ino = ino;
    dir->inode = NULL;
    q_memcpy(dir->name, src->name, src->nameLen);
    dir->name[dir->nameLen] = '\0';
    dir->nameLen = src->nameLen + 1;

    dir->parent = NULL;
    dir->child = NULL;
    list_header_init(&dir->list);
    list_header_init(&dir->link);
    rwlock_init(&dir->rwLock);
    return dir;
}


inode_t *alloc_inode(super_block_t *sb, u16_t mode, const char *name) {
    // 初始化新分配的 inode
    ext2_inode_info_t *info = cache_alloc(&inoCache);
    inode_t *inode = &info->inode;
    u32_t ino = set_inode_bitmap(inode);
    directory_t *dir = alloc_directory(name, ino);

    u32_t time = cur_timestamp();
    u32_t linkCnt, size;
    switch (mode & (~0xfff)) {
        case EXT2_IFREG:
            linkCnt = 1;
            size = 0;
            break;
        case EXT2_IFDIR:
            linkCnt = 2;
            size = sb->blockSize;
            break;
        default: error("unknown_type");
    }

    inode->refCnt = 0;
    inode->mode = mode;
    inode->type = 0;
    inode->linkCnt = linkCnt;
    inode->size = size;
    inode->pageCnt = size / sb->blockSize;
    inode->createTime = time;
    inode->accessTime = time;
    inode->modifiedTime = time;
    inode->deleteTime = 0;
    inode->offset = 0;
    inode->sb = sb;
    inode->ops = &ops;
    inode->dir = dir;
    list_header_init(&inode->list.lru);
    list_header_init(&inode->page);
    rwlock_init(&inode->rwlock);
    q_memset(info->blocks, 0, sizeof(u32_t) * N_BLOCKS);

    return inode;
}

void delete_inode(inode_t *inode) {
    u32_t time = cur_timestamp();
    free_blocks(inode);
    inode->linkCnt = 0;
    inode->deleteTime = time;
    clear_inode_bitmap(inode);
}


static inode_t *root_init(super_block_t *sb) {
    inode_t *root;
    directory_t *dir;

    root = inode_cpy(ROOT_INUM, sb);
    dir = alloc_directory("/", ROOT_INUM);
    dir->inode = root;
    root->dir = dir;

    ext2_open(root);
    return root;
}

static ext2_sb_info_t *superBlock_init() {
    ext2_sb_t *blk;
    groupDesc_t *desc;
    ext2_sb_info_t *sb;
    u32_t groupCount;

    //TODO: 检测文件系统必须实现的功能(requiredFeature)
    //TODO: 检测设备和块号
    blk = ext2_block_read(0)->data + SECTOR_SIZE * SUPER_BLOCK_NO;
    desc = ext2_block_read(1)->data;
    groupCount = blk->blockCnt / blk->blockPerGroup;
    sb = kmalloc(sizeof(ext2_sb_info_t) + sizeof(struct info_descriptor) * groupCount);
    // 初始化 info_descriptor
    for (u64_t i = 0; i < groupCount; ++i) {
        sb->desc[i].dirNum = desc[i].dirNum;
        sb->desc[i].freeBlockCnt = desc[i].freeBlockCnt;
        sb->desc[i].freeInodesCnt = desc[i].freeInodesCnt;
        sb->desc[i].blockBitmap = ext2_block_read(desc[i].blockBitmapAddr)->data;
        sb->desc[i].inodeBitmap = ext2_block_read(desc[i].inodeBitmapAddr)->data;
    }

    // 初始化超级块
    assertk(blk->magic == EXT2_SIGNATURE);
    sb->sb.magic = blk->magic;
    sb->sb.mountTime = blk->mountTime;
    sb->sb.modifiedTime = blk->mountTime;
    sb->sb.freeInodeCnt = blk->freeInodeCnt;
    sb->sb.freeBlockCnt = blk->freeBlockCnt;
    sb->sb.blockSize = (2 << (blk->logBlockSize + 9));
    sb->sb.inodeSize = blk->inodeSize;
    rwlock_init(&sb->sb.rwlock);
    sb->blockPerGroup = blk->blockPerGroup;
    sb->inodePerGroup = blk->inodePerGroup;
    sb->version = blk->fsVersionMajor;
    sb->groupCnt = blk->blockCnt / blk->blockPerGroup;
    return sb;
}


void ext2_init() {
    ext2_sb_info_t *sb;
    inode_t *root;

    ops_init();
    cache_alloc_create(&inoCache, sizeof(ext2_inode_info_t));
    cache_alloc_create(&dirCache, sizeof(directory_t));

    sb = superBlock_init();
    root = root_init(&sb->sb);
    sb->sb.root = root;
    check_superBlock(&sb->sb);
    fs_ops.mount("/", root);
}


void ext2_close(inode_t *inode) {
    inode->refCnt--;
}

inode_t *ext2_find(inode_t *parent, const char *name) {
    ext2_inode_info_t *inode;
    size_t len = q_strlen(name);
    directory_t *dir = parent->dir;
    list_head_t *hdr = &dir->list;

    ext2_open(parent);
    do {
        directory_t *tmp = dir_entry(hdr);
        if (tmp->nameLen == len && q_memcmp(name, tmp->name, len)) {
            if (!dir->inode) {
                inode = ext2_i(inode_cpy(dir->ino, parent->sb));
                if (inode->inode.deleteTime)
                    error("unknown_error");
            }
            ext2_close(parent);
            return &inode->inode;
        }
        hdr = hdr->next;
    } while (hdr != &dir->list);

    ext2_close(parent);
    return NULL;
}


// 读写的数据不会越过 4K (BLOCK_SIZE)边界
u32_t ext2_read(inode_t *file, uint32_t offset, uint32_t size, char *buf) {
    list_head_t *hdr;
    buf_t *page;
    u32_t blockSize = file->sb->blockSize;
    u32_t blkOffset = offset % blockSize;
    u32_t bno = offset / blockSize;
    if (bno >= file->pageCnt) return 0;

    size = MIN(BLOCK_SIZE - blkOffset, size);
    list_for_each(hdr, &file->page) {
        page = page_entry(hdr);
        if (page->index == bno) break;
    }
    if (page_entry(hdr)->index != bno) {
        u32_t bid;
        bid = get_bid(file, bno);
        page = map_block(file, bid, bno);
    }
    q_memcpy(buf, page->data + blkOffset, size);
    return size;
}

u32_t ext2_write(inode_t *file, uint32_t offset, uint32_t size, char *buf) {
    if (!(file->mode & EXT2_IFREG)) goto filetype_error;

    u32_t blockSize = file->sb->blockSize;
    u32_t blkOffset = offset % blockSize;
    u32_t bno = offset / blockSize;
    list_head_t *hdr;
    buf_t *page;

    size = MIN(BLOCK_SIZE - blkOffset, size);

    list_for_each(hdr, &file->page) {
        page = page_entry(hdr);
        if (page->index == bno) break;
    }

    if (page_entry(hdr)->index != bno) {
        u32_t bid;
        // 块在磁盘中但未映射
        if (bno < file->pageCnt) {
            bid = get_bid(file, bno);
            page = map_block(file, bid, bno);
        } else {
            // 映射一个新页缓存,可能会创建空洞文件
            bid = alloc_block(file, bno);
            file->size = MEM_ALIGN(offset, file->sb->blockSize);
            page = ext2_block_read(bid);
        }
    }

    q_memcpy(page->data + blkOffset, buf, size);
    return size;

    filetype_error:
    error("filetype_error");
    return 0; // 返回错误码
}

void ext2_mkdir(inode_t *parent, const char *name) {
    //TODO: 回写时再扩展父目录
    if (!(parent->type & EXT2_IFDIR)) goto file_type_error;
    inode_t *new = ext2_find(parent, name);
    if (new != NULL)
        goto file_exist;

    alloc_inode(parent->sb, EXT2_ALL_RWX | EXT2_IFDIR, name);

    file_exist:
    error("file_exist");
    file_type_error:
    error("file_type_error");
}

void ext2_mkfile(inode_t *parent, const char *name) {
    //TODO: 回写时再扩展父目录

    if (!(parent->type & EXT2_IFDIR)) goto file_type_error;

    inode_t *child = ext2_find(parent, name);
    if (child != NULL)
        goto file_exist;

    alloc_inode(parent->sb, EXT2_ALL_RWX | EXT2_IFREG, name);

    file_exist:
    error("file_exist");
    file_type_error:
    error("file_type_error");
}

void ext2_rmdir(inode_t *inode) {
    directory_t *dir = inode->dir;
    if (!(inode->type & EXT2_IFDIR))
        goto type_error;
    if (dir->child != NULL)
        goto dir_not_empty;

    delete_inode(inode);
    if (dir->parent->child == dir) {
        dir->parent->child = dir_entry(dir->list.next);
    }
    list_del(&dir->list);

    type_error:
    error("type_error");
    dir_not_empty:
    error("dir_not_empty");
}

void ext2_link(inode_t *src, inode_t *parent, const char *name) {
    if (!(src->mode & EXT2_IFREG))
        goto type_error;
    src->linkCnt++;
    directory_t *dir = alloc_directory(name, parent->dir->ino);
    dir->inode = parent;
    list_add_next(&dir->link, &parent->dir->link);

    type_error:
    error("type_error");
}

void ext2_unlink(inode_t *file) {
    directory_t *dir = file->dir;
    if (!(file->type & EXT2_IFREG))
        goto type_error;

    if (file->linkCnt == 1) {
        delete_inode(file);
    }
    file->linkCnt--;
    list_del(&dir->list);

    type_error:
    error("type_error");
}

void ext2_ls(inode_t *inode) {
    directory_t *dir = inode->dir;
    list_head_t *hdr = &dir->list;
    do {
        printfk("%s\n", dir_entry(hdr)->name);
        hdr = hdr->next;
    } while (hdr != &dir->list);
}


static void ops_init() {
    ops.find = ext2_find;
    ops.open = ext2_open;
    ops.close = ext2_close;
    ops.read = ext2_read;
    ops.write = ext2_write;
    ops.mkdir = ext2_mkdir;
    ops.mkfile = ext2_mkfile;
    ops.rmdir = ext2_rmdir;
    ops.link = ext2_link;
    ops.unlink = ext2_unlink;
    ops.ls = ext2_ls;
}