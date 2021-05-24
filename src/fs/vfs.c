//
// Created by pjs on 2021/5/13.
//

#include "fs/vfs.h"
#include "fs/ext2/ext2.h"
#include "sched/tcb.h"
#include "lib/qstring.h"
#include "mm/kmalloc.h"

struct vfs_tree {
    struct vfs_tree *child, *next;

    inode_t *inode;
    char *path;
};


struct vfs_tree *vfs_tree;
struct vfs_ops fs_ops;

static inode_t *cwd(const char *path) {
    if (path[0] == SEPARATOR) {
        assertk(vfs_tree);
        return vfs_tree->inode;
    } else {
        return CUR_TCB->cwd;
    }
}

inode_t *vfs_find(const char *_path) {
    char *path = q_strdup(_path);
    char *cpy = path;

    inode_t *parent = cwd(path), *inode;

    while (path[0]) {
        int i = 0;
        for (; path[i] && path[i] != SEPARATOR; ++i);
        path[i] = '\0';
        inode = parent->ops->find(parent, path);
        if (!inode) {
            kfree(cpy);
            return NULL;
        };
        path = path + i + 1;
    }

    kfree(cpy);
    return inode;
}


void vfs_open(const char *path) {
    inode_t *node = vfs_find(path);
    node->ops->open(node);
    node->refCnt++;
}

void vfs_close(inode_t *node) {
    assertk(node);
    node->refCnt--;
    if (node->refCnt == 0) {
        node->ops->close(node);
    }
}

u32_t vfs_read(inode_t *node, uint32_t size, char *buf) {
    u32_t rdSize;
    int64_t remain = size;
    while (remain >= 0) {
        rdSize = node->ops->read(node, node->offset, remain, buf);
        if (rdSize == 0) break;
        node->offset += rdSize;
        buf += rdSize;
        remain -= rdSize;
    }
    return size - remain;
}

u32_t vfs_write(inode_t *node, uint32_t size, char *buf) {
    u32_t wtSize;
    int64_t remain = size;
    while (remain >= 0) {
        wtSize = node->ops->write(node, node->offset, remain, buf);
        if (wtSize == 0)break;
        node->offset += wtSize;
        buf += wtSize;
        remain -= wtSize;
    }
    return size - remain;
}

void vfs_lseek(inode_t *node, u32_t offset, enum SEEK_WHENCE whence) {
    switch (whence) {
        case SEEK_SET:
            node->offset = offset;
            break;
        case SEEK_END:
            node->offset = node->size + offset;
            break;
        case SEEK_CUR:
            node->offset += offset;
            break;
        default: assertk("seek error");
    }
}


// 返回需要操作的 文件/目录名, parent 为父目录
const char *path_parse(const char *_path, inode_t **parent) {
    char *path = q_strdup(_path);
    u32_t len = q_strlen(path);
    assertk(path[len] != SEPARATOR);
    int64_t i = len;
    for (; i >= 0 && path[len] != SEPARATOR; --i);

    if (i <= 0) {
        *parent = cwd(path);
    } else {
        path[len] = '\0';
        *parent = vfs_find(path);
        assertk(*parent);
    }

    return _path + len + 1;
}

void vfs_mkdir(const char *path) {
    inode_t *parent;
    const char *name = path_parse(path, &parent);
    parent->ops->mkdir(parent, name);
}

void vfs_rmdir(const char *path) {
    inode_t *node = vfs_find(path);
    node->ops->rmdir(node);
}

void vfs_mkfile(const char *path) {
    inode_t *parent;
    const char *name = path_parse(path, &parent);
    parent->ops->mkfile(parent, name);
}


void vfs_unlink(const char *path) {
    inode_t *node = vfs_find(path);
    node->ops->unlink(node);
}


void vfs_link(const char *src, const char *desc) {
    inode_t *sInode = vfs_find(src);
    inode_t *inode;
    const char *name = path_parse(desc, &inode);
    inode->ops->link(sInode, inode, name);
}

void vfs_ls(const char *path) {
    inode_t *inode = vfs_find(path);
    inode->ops->ls(inode);
}

void vfs_umount() {

}

struct vfs_tree *vfs_new_node(inode_t *node, char *path) {
    struct vfs_tree *new = kmalloc(sizeof(struct vfs_tree));
    new->child = NULL;
    new->next = new;
    new->inode = node;
    new->path = path;
    return new;
}

void vfs_mount(char *_path, inode_t *inode) {
    assertk(inode);
    char *path = q_strdup(_path);
    size_t len = q_strlen(_path);
    struct vfs_tree *node = vfs_tree;

    if (vfs_tree == NULL) {
        if (len == 1 && _path[0] == '/') {
            vfs_tree = vfs_new_node(inode, path);
            return;
        }
        assertk("error param");
    }

    if (path[len] == '/') path[len] = '\0';


    int i = 0;
    while (true) {
        for (; node->path[i] && path[i] && node->path[i] == path[i]; ++i);
        if (!node->path[i]) {
            assertk(path[i]);
            if (node->child) {
                node = node->child;
            } else {
                struct vfs_tree *new = vfs_new_node(inode, path);
                node->child = new;
            }
        } else {
            struct vfs_tree *new = vfs_new_node(inode, path);
            new->next = node->next;
            node->next = new;
            return;
        }
    }
}


void vfs_init() {
    vfs_tree = NULL;
    fs_ops.find = vfs_find;
    fs_ops.open = vfs_open;
    fs_ops.close = vfs_close;
    fs_ops.read = vfs_read;
    fs_ops.write = vfs_write;
    fs_ops.lseek = vfs_lseek;
    fs_ops.mkdir = vfs_mkdir;
    fs_ops.rmdir = vfs_rmdir;
    fs_ops.mkfile = vfs_mkfile;
    fs_ops.unlink = vfs_unlink;
    fs_ops.link = vfs_link;
    fs_ops.ls = vfs_ls;
    fs_ops.umount = vfs_umount;
    fs_ops.mount = vfs_mount;
}


#ifdef TEST

void test_vfs() {

}

#endif //TEST
