//
// Created by pjs on 2021/4/20.
//
// ext2 文件系统
// 使用页缓存磁盘原始数据, inoCache,dirCache 缓存 vfs

/*
 * 备忘:
 * inode 号从 1 开始,块号从 0 开始
 * DEBUG:  e2fsck -f disk.img
 * TODO: 权限管理
 */
#include <fs/ext2.h>
#include <fs/writeback.h>
#include <lib/qlib.h>
#include <fs/buf.h>
#include <fs/vfs.h>
#include <lib/qstring.h>
#include <mm/slab.h>
#include <mm/kmalloc.h>

static void ops_init();

static void dir_ops_init();

struct fs_ops ext2_ops;
struct directory_ops ext2_dir_ops;

static inode_t *root_init(super_block_t *sb);


void ext2_init() {
    ext2_sb_info_t *sb;
    inode_t *root;

    ops_init();
    dir_ops_init();
    ext2_dir_init();
    ext2_inode_init();

    sb = superBlock_init();
    root = root_init(&sb->sb);
    sb->sb.root = root;
    vfs_ops.mount("/", root);
}

static inode_t *root_init(super_block_t *sb) {
    inode_t *root;
    directory_t *dir;

    root = inode_cpy(ROOT_INUM, sb);
    dir = directory_alloc("/", ROOT_INUM, NULL, root);
    dir->inode = root;
    dir->sb = root->sb;
    root->dir = dir;
    return root;
}

static void ext2_open(inode_t *inode) {
    assertk(inode);
}

static void ext2_close(inode_t *inode) {
    assertk(inode);
}

static directory_t *ext2_find(inode_t *parent, const char *name) {
    directory_t *dir;

    dir = find_entry_cache(parent, name);
    if (!dir) dir = find_entry_disk(parent, name);
    if (!dir) return NULL;
    return dir;
}

// 读写的数据不会越过 4K (BLOCK_SIZE)边界
static u32_t ext2_read(inode_t *file, uint32_t offset, uint32_t size, char *buf) {
    buf_t *page;
    u32_t blockSize = file->sb->blockSize;
    u32_t blkOffset = offset % blockSize;
    u32_t bno = offset / blockSize;
    if (bno >= DIV_CEIL(file->size, blockSize))
        return 0;

    u32_t bid = get_bid(file, bno);
    page = ext2_block_read(bid, file->sb);

    q_memcpy(buf, page->data + blkOffset, size);
    return size;
}

static u32_t ext2_write(inode_t *file, uint32_t offset, uint32_t size, char *buf) {
    if (!(file->type & EXT2_IFREG)) goto filetype_error;

    u32_t blockSize = file->sb->blockSize;
    u32_t blkOffset = offset % blockSize;
    u32_t bno = offset / blockSize;
    buf_t *page;
    u32_t bid;

    size = MIN(blockSize - blkOffset, size);

    if (bno < get_data_block_cnt(file)) {
        bid = get_bid(file, bno);
        page = ext2_block_read(bid, file->sb);
    } else {
        // 映射一个新页缓存,可能会创建空洞文件
        bid = alloc_block(file, bno);
        page = page_get(bid);
    }

    file->size = MEM_ALIGN(offset, file->sb->blockSize);
    q_memcpy(page->data + blkOffset, buf, size);
    mark_page_dirty(page);
    return size;

    filetype_error:
    error("filetype_error");
    return 0; // 返回错误码
}

static int16_t ext2_mkdir(inode_t *parent, const char *name) {
    if (!ext2_is_dir(parent))
        return -2;
    directory_t *dir = ext2_find(parent, name);
    inode_t *new;
    if (dir != NULL)
        return -1;

    new = inode_alloc(parent, EXT2_ALL_RWX | EXT2_IFDIR, name);
    alloc_block(new, 0);
    make_empty_dir(new);
    append_to_parent(parent, new);
    return 0;
}

static void ext2_mkfile(inode_t *parent, const char *name) {

    if (!ext2_is_dir(parent)) goto file_type_error;

    inode_t *new;
    directory_t *dir = ext2_find(parent, name);
    if (dir != NULL)
        goto file_exist;

    new = inode_alloc(parent, EXT2_ALL_RWX | EXT2_IFREG, name);
    append_to_parent(parent, new);

    file_exist:
    error("file_exist");
    file_type_error:
    error("file_type_error");
}

static void ext2_rmdir(inode_t *inode) {
    directory_t *dir = inode->dir;
    if (!ext2_is_dir(inode))
        goto type_error;
    if (!list_empty(&dir->child))
        goto dir_not_empty;
    if (!dir_empty(inode))
        goto dir_not_empty;

    inode_delete(inode);
    return;

    type_error:
    error("type_error");
    dir_not_empty:
    error("dir_not_empty");
}

static void ext2_link(inode_t *src, inode_t *parent, const char *name) {
    if (!(src->type & EXT2_IFREG))
        goto type_error;
    src->linkCnt++;
    directory_t *dir = directory_alloc(name, src->dir->ino, parent->dir, src);
    list_add_next(&dir->link, &src->dir->link);

    append_link_to_parent(parent, dir, src->type);
    mark_inode_dirty(src, I_DATA);

    type_error:
    error("type_error");
}

static void ext2_unlink(inode_t *parent, directory_t *dir) {
    assertk(dir && dir->sb);
    inode_t *file = dir->inode;
    if (!file)
        file = inode_cpy(dir->ino, dir->sb);

    if (!ext2_is_dir(file))
        goto type_error;

    file->linkCnt--;
    remove_from_parent(parent, dir);
    if (file->linkCnt == 0) {
        inode_delete(file);
    } else {
        mark_inode_dirty(file, I_DATA);
    }

    type_error:
    error("type_error");
}

static void ext2_ls(inode_t *inode) {
    directory_t *dir;
    list_head_t *hdr;

    list_for_each(hdr, &inode->dir->child) {
        dir = dir_brother_entry(hdr);
        char *name = dir_name_dump(dir);
        printfk("%s\n", name);
        kfree(name);
    }
}

void ext2_inode_mount(directory_t *dir) {
    if (!dir->inode) {
        directory_t *parent = dir->parent;
        assertk(parent && dir->sb);
        dir->inode = inode_cpy(dir->ino, dir->sb);
    }
    assertk(dir->inode);
}

extern void ext2_write_back(inode_t *inode);

extern void ext2_write_super_block(super_block_t *_sb);

static void ops_init() {
    ext2_ops.find = ext2_find;
    ext2_ops.open = ext2_open;
    ext2_ops.close = ext2_close;
    ext2_ops.read = ext2_read;
    ext2_ops.write = ext2_write;
    ext2_ops.mkdir = ext2_mkdir;
    ext2_ops.mkfile = ext2_mkfile;
    ext2_ops.rmdir = ext2_rmdir;
    ext2_ops.link = ext2_link;
    ext2_ops.unlink = ext2_unlink;
    ext2_ops.ls = ext2_ls;
    ext2_ops.write_back = ext2_write_back;
    ext2_ops.write_super_block = ext2_write_super_block;
}

static void dir_ops_init() {
    ext2_dir_ops.inode_mount = ext2_inode_mount;
}