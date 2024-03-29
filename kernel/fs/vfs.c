//
// Created by pjs on 2021/5/13.
//
// TODO: 所有需要操作的父目录与文件添加引用计数,防止被回收
// TODO: 挂载
#include <fs/vfs.h>
#include <task/task.h>
#include <lib/qstring.h>
#include <mm/kmalloc.h>
#include <fs/writeback.h>
#include <fs/hash.h>

inode_t *vfs_find(char *path);

static char *split_path(char *path, inode_t **parent);

static char *parse_path(const char *path);

static directory_t *find_dir(char *path);

static void add_open_file(inode_t *inode, fd_t fd);

static struct open_file *remove_open_file(inode_t *inode);

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
        assertk(CUR_TCB->cwd);
        return get_inode(CUR_TCB->cwd);
    }
}

fd_t vfs_open(const char *_path) {
    int32_t error;
    inode_t *inode;
    char *path;

    path = parse_path(_path);

    inode = vfs_find(path);
    if (!inode) return -1;

    fd_t fd = alloc_fd(inode);
    if (fd < 0)return -1;

    error = inode->ops->open(inode);
    if (error != 0) return error;
    inode->refCnt++;

    kfree(path);

    add_open_file(inode, fd);
    return fd;
}

int32_t vfs_close(fd_t fd) {
    int32_t error;
    inode_t *node = get_inode(fd);

    if (!node) return -1;
    node->refCnt--;
    struct open_file *file = remove_open_file(node);

    if (node->refCnt == 0) {
        kfree(file);
        free_fd(fd);
        error = node->ops->close(node);
        if (error != 0) return error;
    }
    return 0;
}


u32_t vfs_read(fd_t fd, void *buf, size_t size) {
    inode_t *node = get_inode(fd);
    if (!node) return 0;

    u32_t rdSize;
    int64_t remain = size;
    while (remain > 0) {
        rdSize = node->ops->read(node, node->offset, remain, buf);
        if (rdSize == 0) break;
        node->offset += rdSize;
        buf += rdSize;
        remain -= rdSize;
    }
    return size - remain;
}

struct page *vfs_read_page(fd_t fd, size_t offset) {
    inode_t *node = get_inode(fd);
    return node->ops->read_page(node, offset);
}

u32_t vfs_write(fd_t fd, void *buf, size_t size) {
    inode_t *node = get_inode(fd);
    if (!node) return 0;

    u32_t wtSize;
    int64_t remain = size;

    while (remain > 0) {
        wtSize = node->ops->write(node, node->offset, remain, buf);
        if (wtSize == 0)break;
        node->offset += wtSize;
        buf += wtSize;
        remain -= wtSize;
    }
    return size - remain;
}

int32_t vfs_lseek(fd_t fd, int32_t offset, enum SEEK_WHENCE whence) {
    inode_t *node = get_inode(fd);
    if (!node) return -2;

    int64_t end;
    switch (whence) {
        case SEEK_SET:
            if (offset < 0) return -1;
            node->offset = offset;
            break;
        case SEEK_END:
            end = node->size + offset;
            if (end < 0)
                return -1;
            node->offset = node->size + offset;
            break;
        case SEEK_CUR:
            end = node->offset + offset;
            if (end < 0)
                return -1;
            node->offset = end;
            break;
        default: error("seek error");
    }
    return node->offset;
}

int64_t vfs_ftell(fd_t fd) {
    inode_t *node = get_inode(fd);
    if (!node) return -2;
    return node->offset;
}


int32_t vfs_mkdir(const char *_path) {
    inode_t *parent;
    int32_t error;
    char *path, *name;

    path = parse_path(_path);
    name = split_path(path, &parent);
    if (!parent) return -1;
    error = parent->ops->mkdir(parent, name);
    if (error != 0) return error;

    kfree(path);
    return 0;
}


int32_t vfs_rmdir(const char *_path) {
    int32_t error;
    inode_t *node;
    char *path;

    path = parse_path(_path);
    node = vfs_find(path);
    if (!node) return -1;
    error = node->ops->rmdir(node);
    if (error != 0) return error;
    kfree(path);
    return 0;
}

int32_t vfs_mkfile(const char *_path) {
    inode_t *parent;
    int32_t error;
    char *path, *name;

    path = parse_path(_path);
    name = split_path(path, &parent);
    if (!parent) return -1;
    error = parent->ops->mkfile(parent, name);
    if (error != 0) return error;
    kfree(path);
    return 0;
}


int32_t vfs_unlink(const char *_path) {
    int32_t error;
    directory_t *file;
    inode_t *inode;
    char *path;

    path = parse_path(_path);

    file = find_dir(path);
    if (!file) return -2;

    file->ops->inode_mount(file);
    inode = file->inode;
    assertk(inode);

    error = inode->ops->unlink(file);
    if (error != 0) return error;
    kfree(path);
    return 0;
}

int32_t vfs_link(const char *src, const char *desc) {
    int32_t error;
    inode_t *inode, *sInode;
    char *n_src, *n_desc, *name;

    n_src = parse_path(src);
    n_desc = parse_path(desc);

    sInode = vfs_find(n_src);
    if (!sInode) return -4;

    name = split_path(n_desc, &inode);
    if (!inode) return -3;

    error = inode->ops->link(sInode, inode, name);
    if (error != 0) return error;

    kfree(n_desc);
    kfree(n_src);
    return 0;
}

void vfs_ls(const char *_path) {
    char *path = parse_path(_path);

    inode_t *inode = vfs_find(path);
    assertk(inode);
    inode->ops->ls(inode);

    kfree(path);
}

static directory_t *find_dir(char *path) {

    inode_t *parent = cwd(path);
    directory_t *dir = parent->dir;

    if (path[0] == SEPARATOR)
        path++;

    while (path[0]) {
        int i = 0;
        for (; path[i] && path[i] != SEPARATOR; ++i);
        if (!path[i]) {
            dir = parent->ops->find(dir, path);
            break;
        }
        path[i] = '\0';
        dir = parent->ops->find(dir, path);
        if (!dir) return NULL;
        path = path + i + 1;
    }

    return dir;
}

inode_t *vfs_find(char *path) {
    directory_t *dir = find_dir(path);
    if (!dir) return NULL;
    assertk(dir->ops);
    dir->ops->inode_mount(dir);
    return dir->inode;
}

static char *parse_path(const char *path) {
    u32_t len = strlen(path) + 1;
    char *new = kmalloc(len);
    memcpy(new, path, len + 1);
    if (new[len - 1] == SEPARATOR) {
        new[len - 1] = '\0';
    }
    return new;
}

// 返回需要操作的 文件/目录名, parent 为父目录
static char *split_path(char *path, inode_t **parent) {
    u32_t len = strlen(path);
    int64_t i = len;

    for (; i >= 0 && path[i] != SEPARATOR; --i);

    if (i <= 0) {
        *parent = cwd(path);
    } else {
        path[i] = '\0';
        *parent = vfs_find(path);
        if (!(*parent)) return NULL;
    }
    return path + i + 1;
}


char *dir_name_dump(directory_t *dir) {
    char *name = kmalloc(dir->nameLen + 1);
    u32_t cpy = MIN(dir->nameLen, FILE_DNAME_LEN);
    memcpy(name, dir->l_name, cpy);
    if (cpy > FILE_DNAME_LEN) {
        memcpy(name + dir->nameLen - cpy, dir->h_name, dir->nameLen - cpy);
    }
    name[dir->nameLen] = '\0';
    return name;
}

void dir_name_set(directory_t *dir, const char *name, u32_t len) {
    assertk(len <= FILE_NAME_LEN);
    u32_t cpy = MIN(len, FILE_DNAME_LEN);

    memcpy(dir->l_name, name, cpy);
    dir->h_name = NULL;
    dir->nameLen = len;
    if (len > FILE_DNAME_LEN) {
        dir->h_name = kmalloc(len - cpy);
        memcpy(dir->h_name, name + cpy, len - cpy);
    }
}

bool dir_name_cmp(directory_t *dir, const char *name) {
    u32_t len = MIN(FILE_DNAME_LEN, dir->nameLen);
    if (!memcmp(dir->l_name, name, len)) {
        return false;
    }

    if (len > FILE_DNAME_LEN) {
        assertk(dir->h_name);
        if (!memcmp(dir->h_name, name + len, dir->nameLen - len))
            return false;
    }
    return true;
}

static void add_open_file(inode_t *inode, fd_t fd) {
    for (struct open_file *file = CUR_TCB->open; file; file = file->next) {
        if (file->inode == inode) {
            assertk(file->fd == fd);
            return;
        }
    }

    struct open_file *open = kmalloc(sizeof(struct open_file));
    open->fd = fd;
    open->inode = inode;
    open->next = CUR_TCB->open;
    CUR_TCB->open = open;
}

static struct open_file *remove_open_file(inode_t *inode) {
    struct open_file *file, *prev = NULL;
    for (file = CUR_TCB->open; file; prev = file, file = file->next) {
        if (file->inode == inode) {
            if (prev == NULL) {
                CUR_TCB->open = file->next;
            } else {
                prev->next = file->next;
            }
            return file;
        }
    }
    assertk(0);
    return NULL;
}

void copy_open_file(struct open_file **src, struct open_file **desc) {
    struct open_file *file;
    *desc = *src;
    for (file = *src; file; file = file->next) {
        file->inode->refCnt++;
    }
}

void recycle_open_file(struct open_file *open) {
    assertk(open);
    struct open_file *file, *next;
    for (file = open; file; file = next) {
        next = file->next;
        vfs_close(file->fd);
    }
}

void getcwd(UNUSED char *buf, UNUSED size_t size) {

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
    size_t len = strlen(_path);

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


fd_t _vfs_find(char *path) {
    inode_t *inode = vfs_find(path);
    fd_t fd;
    assertk(inode);

    fd = alloc_fd(inode);
    assertk(fd >= 0);

    add_open_file(inode, fd);

    return fd;
}

void vfs_init() {
    vfs_tree = NULL;
    vfs_ops.find = _vfs_find;
    vfs_ops.open = vfs_open;
    vfs_ops.close = vfs_close;
    vfs_ops.read = vfs_read;
    vfs_ops.read_page = vfs_read_page;
    vfs_ops.ftell = vfs_ftell;
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
UNUSED static char charBuf[4098] = "11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111";

UNUSED void test_vfs() {
//    inode_t *zsh = vfs_ops.open("/zsh");
//    assertk(zsh != NULL);
//    assertk(vfs_ops.lseek(zsh, 880640, SEEK_SET) >= 0);
////    assertk(vfs_ops.read(zsh, 4098, charBuf) != 0);
//    assertk(vfs_ops.write(zsh, 4098, charBuf) != 0);
//    assertk(vfs_ops.unlink("/zsh") == 0);
//    vfs_ops.close(zsh);
//    page_fsync();


    assertk(vfs_ops.mkdir("/foo") == 0);
    assertk(vfs_ops.mkdir("/foo2") == 0);
    assertk(vfs_ops.mkdir("/foo3") == 0);

    assertk(vfs_ops.rmdir("/foo3") == 0);


    char path[] = "/foo/txt1";
    assertk(find_dir(path) == NULL);

    assertk(vfs_ops.mkfile("/foo/txt") == 0);

    // 测试写
    fd_t txt = vfs_ops.open("/foo/txt");
    assertk(txt >= 0);
    vfs_ops.write(txt, charBuf, 4098);
    assertk(vfs_ops.close(txt) == 0);

    // 测试读,需要预先创建文件
    fd_t reader = vfs_ops.open("/txt");
    assertk(reader >= 0);
    vfs_ops.read(reader, charBuf, 4098);
    assertk(vfs_ops.close(reader) == 0);

    assertk(vfs_ops.link("/foo/txt", "/foo2/hh") == 0);
    assertk(vfs_ops.link("/txt", "/foo2/txt2") == 0);

    assertk(vfs_ops.unlink("/foo2/txt2") == 0);
    page_fsync();
}

#endif //TEST
