//
// Created by pjs on 2021/5/26.
//
#include <types.h>
#include <fs/vfs.h>
#include <fs/ext2.h>
#include <lib/qstring.h>
#include <fs/writeback.h>
#include <mm/slab.h>
#include <mm/kmalloc.h>

#define DIR_END(dir, blockSize)            (((dir) & (~((blockSize)-1))) + (blockSize))

#define for_each_dir(dir, blockSize)       for(ext2_dir_t* end = (void *) DIR_END((ptr_t)(dir),blockSize); (dir) < end; (dir) = next_entry(dir))

#define ALIGN4(size)            MEM_ALIGN(size, 4)

#define dir_len(dir)            (ALIGN4(sizeof(ext2_dir_t) + (dir)->nameLen))


static struct slabCache dirCache;

void ext2_dir_init() {
    cache_alloc_create(&dirCache, sizeof(directory_t));
}

INLINE ext2_dir_t *next_entry(ext2_dir_t *dir) {
    return (void *) dir + dir->entrySize;
}

void directory_destroy(directory_t *dir) {
    cache_free(&dirCache, dir);
}

directory_t *directory_get() {
    directory_t *dir = cache_alloc(&dirCache);
    assertk(dir);
    return dir;
}

directory_t *directory_alloc(
        const char *name, u32_t ino,
        directory_t *parent, inode_t *inode) {

    directory_t *dir = directory_get();
    dir->ino = ino;
    dir->inode = inode;
    dir_name_set(dir, name, strlen(name));

    dir->sb = inode->sb;
    dir->ops = &ext2_dir_ops;
    dir->parent = parent;
    list_header_init(&dir->child);
    list_header_init(&dir->brother);
    list_header_init(&dir->link);
    rwlock_init(&dir->rwLock);
    return dir;
}

directory_t *directory_cpy(ext2_dir_t *src, directory_t *parent) {
    directory_t *dir = directory_get();
    dir->ino = src->inode;
    dir->inode = NULL;
    dir->ops = &ext2_dir_ops;
    dir_name_set(dir, src->name, src->nameLen);

    assertk(parent->sb);
    dir->sb = parent->sb;
    dir->parent = parent;
    list_header_init(&dir->child);
    list_header_init(&dir->brother);
    list_header_init(&dir->link);
    rwlock_init(&dir->rwLock);
    return dir;
}

directory_t *find_entry_cache(directory_t *parent, const char *name) {
    size_t len = strlen(name);
    list_head_t *hdr;

    list_for_each(hdr, &parent->child) {
        directory_t *tmp = dir_brother_entry(hdr);
        if (tmp->nameLen == len && dir_name_cmp(tmp, name))
            return tmp;
    }

    return NULL;
}


directory_t *find_entry_disk(inode_t *parent, const char *name) {
    size_t name_len = strlen(name);
    ext2_dir_t *dir;
    u32_t bid;
    for_each_block(bid, parent) {
        dir = ext2_block_read(bid, parent->sb)->data;
        for_each_dir(dir, parent->sb->blockSize) {
            if (dir->inode != 0
                && dir->nameLen == name_len &&
                memcmp(name, dir->name, name_len)) {

                directory_t *new = directory_cpy(dir, parent->dir);
                return new;
            }
        }
    }
    return NULL;
}

// 向后合并多个 inode 为 0 的目录项
static void merge_dir_entry(ext2_dir_t *dir, u32_t blockSize) {
    assertk(dir);

    if (dir->inode != 0) return;

    size_t size = 0;
    ext2_dir_t *hdr = dir;
    for_each_dir(hdr, blockSize) {
        if (hdr->inode != 0)break;
        size += hdr->entrySize;
    }
    dir->entrySize = size;
}

static ext2_dir_t *new_dir_entry(ext2_dir_t *dir, u32_t ino, char *name, size_t name_len, u8_t type) {
    assertk(name_len < FILE_NAME_LEN);
    u32_t entrySize = ALIGN4(name_len + sizeof(ext2_dir_t));
    u32_t dirLen = ALIGN4(dir_len(dir));
    ext2_dir_t *new = dir;

    if (dir->inode != 0) {
        if (dir->entrySize - dirLen < entrySize)
            return NULL;
        new = (void *) dir + dirLen;
        new->entrySize = dir->entrySize - dirLen;
        dir->entrySize = dirLen;
    }

    if (entrySize > new->entrySize) return NULL;

    new->inode = ino;
    new->nameLen = name_len;
    new->type = type;
    memcpy(new->name, name, name_len);
    u32_t remain = new->entrySize - entrySize;
    if (remain > ALIGN4(sizeof(ext2_dir_t) + 1)) {
        new->entrySize = entrySize;
        new = (void *) new + entrySize;
        new->inode = 0;
        new->entrySize = remain;
    }
    return new;
}


static void create_dir_entry(inode_t *parent, char *name, u32_t ino, u8_t type) {
    ext2_dir_t *dir, *new;
    u32_t bid;
    struct page *buf;
    u32_t blockSize = parent->sb->blockSize;
    size_t nameLen = strlen(name);

    for_each_block(bid, parent) {
        buf = ext2_block_read(bid, parent->sb);
        dir = buf->data;
        for_each_dir(dir, blockSize) {
            if ((new = new_dir_entry(dir, ino, name, nameLen, type))) {
                merge_dir_entry(new, blockSize);
                mark_page_dirty(buf);
                return;
            }
        }
    }

    bid = alloc_block(parent, next_free_bno(parent));
    parent->size += blockSize;
    buf = ext2_block_read(bid, parent->sb);

    dir = buf->data;
    dir->inode = 0;
    dir->entrySize = blockSize;
    new_dir_entry(dir, ino, name, nameLen, type);
    mark_page_dirty(buf);
}

void make_empty_dir(inode_t *inode) {
    // 创建 . 与 .. 条目
    struct page *buf;
    ext2_dir_t *dir;
    u32_t bid = ext2_i(inode)->blocks[0];
    u32_t blockSize = inode->sb->blockSize;
    assertk(bid);

    buf = ext2_block_read(bid, inode->sb);
    dir = buf->data;

    dir->inode = 0;
    dir->entrySize = blockSize;
    dir = new_dir_entry(dir, inode->dir->ino, ".", 1, EXT2_FT_DIR);
    new_dir_entry(dir, inode->dir->parent->ino, "..", 2, EXT2_FT_DIR);
    mark_page_dirty(buf);
}

u32_t type_inode2dir(u32_t type) {
    static u32_t map[7] = {
            EXT2_FT_FIFO, EXT2_FT_CHRDEV, EXT2_FT_DIR, EXT2_FT_BLKDEV,
            EXT2_FT_REG_FILE, EXT2_FT_SYMLINK, EXT2_FT_SOCK
    };
    u32_t index = type >> 13;
    if (index > 7) return EXT2_FT_UNKNOWN;
    return map[index];
}

void append_link_to_parent(inode_t *parent, directory_t *dir, u16_t _type) {
    u16_t type = type_inode2dir(_type);
    char *name = dir_name_dump(dir);
    create_dir_entry(parent, name, dir->ino, type);
    list_add_prev(&dir->brother, &parent->dir->child);

    kfree(name);
}

void append_to_parent(inode_t *parent, inode_t *child) {
    directory_t *dir = child->dir;
    u16_t type = type_inode2dir(child->type);
    char *name = dir_name_dump(dir);
    create_dir_entry(parent, name, dir->ino, type);
    list_add_prev(&dir->brother, &parent->dir->child);

    if (ext2_is_dir(child)) {
        parent->linkCnt++;
        mark_inode_dirty(parent, I_DATA);
    }

    kfree(name);
}

bool dir_empty(inode_t *inode) {
    u32_t bid;
    ext2_dir_t *dir;
    struct page *buf;
    u32_t blockSize = inode->sb->blockSize;
    assertk(ext2_is_dir(inode));
    if (inode->linkCnt != 2) {
        return false;
    }
    for_each_block(bid, inode) {
        buf = ext2_block_read(bid, inode->sb);
        dir = buf->data;
        for_each_dir(dir, blockSize) {
            if (dir->inode != 0) {
                if (dir->name[0] != '.')
                    return false;
                if (dir->nameLen == 1)
                    continue;
                if (dir->nameLen == 2 && dir->name[1] == '.')
                    continue;
                return false;
            }
        }
    }
    return true;
}

void remove_from_parent(inode_t *parent, directory_t *target) {
    assertk(parent && target);

    ext2_dir_t *dir;
    u32_t bid;
    struct page *buf;
    u32_t blockSize = parent->sb->blockSize;
    size_t name_len = target->nameLen;
    char *name = dir_name_dump(target);

    for_each_block(bid, parent) {
        buf = ext2_block_read(bid, parent->sb);
        dir = buf->data;
        for_each_dir(dir, blockSize) {
            if (dir->inode != 0
                && dir->nameLen == name_len
                && memcmp(name, dir->name, dir->nameLen)) {

                dir->inode = 0;
                merge_dir_entry(dir, blockSize);
                mark_page_dirty(buf);
                return;
            }
        }
    }
}

