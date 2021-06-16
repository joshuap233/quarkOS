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
#include <mm/page.h>
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

static int32_t ext2_open(inode_t *inode) {
    if (!inode) return -1;
    return 0;
}

static int32_t ext2_close(inode_t *inode) {
    if (!inode) return -1;
    if (ext2_is_dir(inode) && inode->offset != 0) {
        inode->offset = 0;
    }
    return 0;
}

static directory_t *ext2_find(directory_t *parent, const char *name) {
    directory_t *dir;

    dir = find_entry_cache(parent, name);
    if (dir) return dir;

    // 需要到页缓存(磁盘)查找目录项时,将 inode 拷贝到缓存
    parent->ops->inode_mount(parent);
    // 将 directory_t 缓存到 dir->child 时,
    // 查找则可以不用从页缓存找 inode,因此可以将 inode 回收
    dir = find_entry_disk(parent->inode, name);
    if (dir) {
        list_add_prev(&dir->brother, &parent->child);
    };
    return dir;
}

// 读写的数据不会越过 4K (BLOCK_SIZE)边界
static u32_t ext2_read(inode_t *file, uint32_t offset, uint32_t size, char *buf) {
    struct page*page;
    u32_t blockSize = file->sb->blockSize;
    u32_t blkOffset = offset % blockSize;
    u32_t bno = offset / blockSize;

    if (bno >= DIV_CEIL(file->size, blockSize))
        return 0;

    size = MIN(blockSize - blkOffset, size);
    u32_t bid = get_bid(file, bno);
    if (!bid) return 0;

    page = ext2_block_read(bid, file->sb);

    q_memcpy(buf, page->data + blkOffset, size);
    return size;
}

static u32_t ext2_write(inode_t *file, uint32_t offset, uint32_t size, char *buf) {
    if (!(file->type & EXT2_IFREG)) goto filetype_error;

    u32_t blockSize = file->sb->blockSize;
    u32_t blkOffset = offset % blockSize;
    u32_t bno = offset / blockSize;
    struct page*page;
    u32_t bid;


    size = MIN(blockSize - blkOffset, size);

    if (bno < next_free_bno(file)) {
        bid = get_bid(file, bno);
        page = ext2_block_read(bid, file->sb);
    } else {
        // 映射一个新页缓存,可能会创建空洞文件
        bid = alloc_block(file, bno);
        page = ext2_block_get(bid, file->sb);
    }

    file->size = offset + size;
    q_memcpy(page->data + blkOffset, buf, size);
    mark_page_dirty(page);
    mark_inode_dirty(file, I_DATA);
    return size;

    filetype_error:
    error("filetype_error");
    return 0; // 返回错误码
}

static int32_t ext2_mkdir(inode_t *parent, const char *name) {
    if (!ext2_is_dir(parent))
        return -1;
    directory_t *dir = ext2_find(parent->dir, name);
    inode_t *new;
    if (dir != NULL)
        return -2;

    new = inode_alloc(parent, EXT2_ALL_RWX | EXT2_IFDIR, name);
    alloc_block(new, 0);
    new->size = new->sb->blockSize;

    make_empty_dir(new);
    append_to_parent(parent, new);
    mark_inode_dirty(new, I_NEW);
    return 0;
}

static int32_t ext2_mkfile(inode_t *parent, const char *name) {
    if (!ext2_is_dir(parent))
        return -1;

    inode_t *new;
    directory_t *dir = ext2_find(parent->dir, name);
    if (dir != NULL)
        return -2;

    new = inode_alloc(parent, EXT2_ALL_RWX | EXT2_IFREG, name);
    append_to_parent(parent, new);
    mark_inode_dirty(new,I_NEW);
    return 0;
}

static int32_t ext2_rmdir(inode_t *inode) {
    directory_t *dir = inode->dir;
    inode_t *parent;

    assertk(dir && dir->parent);
    if (!ext2_is_dir(inode))
        return -1;
    if (!list_empty(&dir->child))
        return -2;
    if (!dir_empty(inode))
        return -2;


    ext2_inode_mount(dir->parent);
    parent = dir->parent->inode;

    remove_from_parent(parent, inode->dir);
    inode_delete(inode);

    parent->linkCnt--;
    mark_inode_dirty(parent, I_DATA);
    return 0;
}

static int32_t ext2_link(inode_t *src, inode_t *parent, const char *name) {
    if (!(src->type & EXT2_IFREG))
        return -1;
    src->linkCnt++;
    directory_t *dir = directory_alloc(name, src->dir->ino, parent->dir, src);
    list_add_next(&dir->link, &src->dir->link);

    append_link_to_parent(parent, dir, src->type);
    mark_inode_dirty(src, I_DATA);
    return 0;
}

static int32_t ext2_unlink(directory_t *file) {
    assertk(file && file->parent);
    inode_t *inode, *parent;

    ext2_inode_mount(file);
    if (ext2_is_dir(file->inode))
        return -1;

    ext2_inode_mount(file->parent);
    inode = file->inode;
    parent = file->parent->inode;


    inode->linkCnt--;
    remove_from_parent(parent, file);
    if (inode->linkCnt == 0) {
        inode_delete(inode);
        return 0;
    }

    mark_inode_dirty(inode, I_DATA);
    return 0;
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
    inode_t *inode;
    if (!dir->inode) {
        directory_t *parent = dir->parent;
        assertk(parent && dir->sb);
        inode = inode_cpy(dir->ino, dir->sb);
        dir->inode = inode;
        if (!inode->dir) {
            inode->dir = dir;
        } else if (inode->dir != dir) {
            list_add_prev(&dir->link, &inode->dir->link);
        }
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