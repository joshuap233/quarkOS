//
// Created by pjs on 2021/5/13.
//
// TODO: 所有需要操作的父目录与文件添加引用计数,防止被回收
#include <fs/vfs.h>
#include <sched/tcb.h>
#include <lib/qstring.h>
#include <mm/kmalloc.h>
#include <fs/writeback.h>

inode_t *vfs_find(char *path);

static char *split_path(char *path, inode_t **parent);

static char *parse_path(const char *path);

static directory_t *find_dir(char *path);

//记录挂载点
struct vfs_tree {
    struct vfs_tree *child, *next;

    directory_t *dir;
};

struct vfs_tree *vfs_tree;
struct vfs_ops vfs_ops;

static inode_t *cwd(const char *path) {
    if (path[0] == SEPARATOR) {
        assertk(vfs_tree && vfs_tree->dir && vfs_tree->dir->inode);
        return vfs_tree->dir->inode;
    } else {
        return CUR_TCB->cwd;
    }
}

inode_t *vfs_open(const char *_path) {
    char *path = parse_path(_path);

    inode_t *inode = vfs_find(path);
    assertk(inode);
    inode->ops->open(inode);
    inode->refCnt++;

    kfree(path);
    return inode;
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
        default: error("seek error");
    }
}

void vfs_mkdir(const char *_path) {
    inode_t *parent;
    char *path = parse_path(_path);

    char *name = split_path(path, &parent);
    parent->ops->mkdir(parent, name);

    kfree(path);
}


void vfs_rmdir(const char *_path) {
    char *path = parse_path(_path);

    inode_t *node = vfs_find(path);
    assertk(node);
    node->ops->rmdir(node);

    kfree(path);
}

void vfs_mkfile(const char *_path) {
    inode_t *parent;
    char *path = parse_path(_path);

    char *name = split_path(path, &parent);
    parent->ops->mkfile(parent, name);

    kfree(path);
}


void vfs_unlink(const char *_path) {
    inode_t *parent;
    char *path = parse_path(_path);

    char *name = split_path(path, &parent);
    directory_t *dir = find_dir(name);
    assertk(dir);
    parent->ops->unlink(parent, dir);

    kfree(path);
}

void vfs_link(const char *src, const char *desc) {
    char *n_src = parse_path(src);
    char *n_desc = parse_path(desc);

    inode_t *sInode = vfs_find(n_src);
    assertk(sInode);
    inode_t *inode;
    char *name = split_path(n_desc, &inode);
    inode->ops->link(sInode, inode, name);

    kfree(n_desc);
    kfree(n_src);
}

void vfs_ls(const char *_path) {
    char *path = parse_path(_path);

    inode_t *inode = vfs_find(path);
    assertk(inode);
    inode->ops->ls(inode);

    kfree(path);
}

static directory_t *find_dir(char *path) {
    int i = 0;

    inode_t *parent = cwd(path);
    directory_t *dir;

    while (path[0]) {
        if (path[0] == SEPARATOR)
            path++;
        for (; path[i] && path[i] != SEPARATOR; ++i);
        path[i] = '\0';
        dir = parent->ops->find(parent, path);
        if (!dir) return NULL;
        path = path + i;
    }

    return dir;
}

inode_t *vfs_find(char *path) {
    directory_t *dir = find_dir(path);
    assertk(dir && dir->ops);
    dir->ops->inode_mount(dir);
    return dir->inode;
}

static char *parse_path(const char *path) {
    u32_t len = q_strlen(path) + 1;
    char *new = kmalloc(len);
    q_memcpy(new, path, len + 1);
    if (new[len - 1] == SEPARATOR) {
        new[len - 1] = '\0';
    }
    return new;
}

// 返回需要操作的 文件/目录名, parent 为父目录
static char *split_path(char *path, inode_t **parent) {
    u32_t len = q_strlen(path);
    int64_t i = len;

    for (; i >= 0 && path[i] != SEPARATOR; --i);

    if (i <= 0) {
        *parent = cwd(path);
    } else {
        path[i] = '\0';
        *parent = vfs_find(path);
        assertk(*parent);
    }
    return path + i + 1;
}


char *dir_name_dump(directory_t *dir) {
    char *name = kmalloc(dir->nameLen + 1);
    u32_t cpy = MIN(dir->nameLen, FILE_DNAME_LEN);
    q_memcpy(name, dir->l_name, cpy);
    if (cpy > FILE_DNAME_LEN) {
        q_memcpy(name + dir->nameLen - cpy, dir->h_name, dir->nameLen - cpy);
    }
    name[dir->nameLen] = '\0';
    return name;
}

void dir_name_set(directory_t *dir, const char *name) {
    u32_t len = q_strlen(name);
    u32_t cpy = MIN(len, FILE_DNAME_LEN);
    assertk(len <= FILE_NAME_LEN);

    q_memcpy(dir->l_name, name, cpy);
    dir->h_name = NULL;
    dir->nameLen = len;
    if (len > FILE_DNAME_LEN) {
        dir->h_name = kmalloc(len - cpy);
        q_memcpy(dir->h_name, name + cpy, len - cpy);
    }
}

bool dir_name_cmp(directory_t *dir, const char *name) {
    u32_t len = MIN(FILE_DNAME_LEN, dir->nameLen);
    if (!q_memcmp(dir->l_name, name, len)) {
        return false;
    }

    if (len > FILE_DNAME_LEN) {
        assertk(dir->h_name);
        if (!q_memcmp(dir->h_name, name + len, dir->nameLen - len))
            return false;
    }
    return true;
}

/*----  挂载 ----*/
directory_t *get_mount_point(UNUSED char *path) {
    //TODO
    return vfs_tree->dir;
}

struct vfs_tree *vfs_new_node(directory_t *dir) {
    struct vfs_tree *new = kmalloc(sizeof(struct vfs_tree));
    new->child = NULL;
    new->next = NULL;
    new->dir = dir;
    return new;
}

void vfs_mount(char *_path, inode_t *inode) {
    assertk(inode);
    size_t len = q_strlen(_path);

    if (vfs_tree == NULL) {
        if (len == 1 && _path[0] == '/') {
            vfs_tree = vfs_new_node(inode->dir);
            return;
        }
        error("error param");
    }
    //TODO:挂载
}

void vfs_umount() {
    //TODO:
}

void vfs_init() {
    vfs_tree = NULL;
    vfs_ops.open = vfs_open;
    vfs_ops.close = vfs_close;
    vfs_ops.read = vfs_read;
    vfs_ops.write = vfs_write;
    vfs_ops.lseek = vfs_lseek;
    vfs_ops.mkdir = vfs_mkdir;
    vfs_ops.rmdir = vfs_rmdir;
    vfs_ops.mkfile = vfs_mkfile;
    vfs_ops.unlink = vfs_unlink;
    vfs_ops.link = vfs_link;
    vfs_ops.ls = vfs_ls;
    vfs_ops.umount = vfs_umount;
    vfs_ops.mount = vfs_mount;
}


#ifdef TEST
static char charBuf[4097] = "";

void test_vfs() {
    vfs_ops.mkdir("/foo");
    vfs_ops.mkdir("/foo2");
    vfs_ops.mkdir("/foo3");

    vfs_ops.rmdir("/foo3");
    page_fsync();

    // 测试读,需要预先创建文件
    inode_t *read = vfs_ops.open("/txt");
    vfs_ops.read(read, 4097, charBuf);

    // 测试写
    vfs_ops.mkfile("/foo/txt");
    inode_t *txt = vfs_ops.open("/foo/txt");
    vfs_ops.write(txt, 4097, charBuf);

    vfs_ops.link("/foo/txt", "/foo2/hh");
    vfs_ops.unlink("/foo/txt");

}

#endif //TEST
