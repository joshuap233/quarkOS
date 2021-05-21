//
// Created by pjs on 2021/5/13.
//
// 下面的代码只是对 ext2 的简单封装,提供更高层次的接口

// TODO: 目录树与挂载,
// TODO: 将文件系统操作函数定义在 vfs_node 结构中

#include "fs/vfs.h"
#include "fs/ext2.h"
#include "sched/tcb.h"
#include "mm/kmalloc.h"
#include "lib/qstring.h"


inode_t *root = NULL;

typedef struct vfs_directory {
    inode_t *parent;
    char *name;
} vfs_dir_t;

void vfs_init() {
    root = get_root();
}

// 分离父目录与需要操作的文件
void vfs_parse_path(char *_path, vfs_dir_t *dir) {
    char *path = q_strdup(_path);
    assertk(path);

    inode_t *cwd;
    u32_t sep = 0;
    int i = 0;

    for (; path[i]; ++i) {
        if (path[i] == SEPARATOR)
            sep = i;
    }
    assertk(i != 0 && path[i - 1] != '/');

    if (sep == 0) {
        dir->parent = root;
    } else {
        path[sep] = '\0';
        cwd = path[0] == '/' ? root : CUR_TCB->cwd;
        dir->parent = ext2_find(cwd, path);
    }
    dir->name = &_path[i + 1];
    kfree(path);
}

void vfs_open(char *path) {
    inode_t *cwd = path[0] == '/' ? root : CUR_TCB->cwd;
    inode_t *node = ext2_find(cwd, path);
    ext2_open(node);
}

void vfs_close(inode_t *node) {
    assertk(node);
    node->ref_cnt--;
    if (node->ref_cnt == 0) {
        ext2_close(node);
    }
}

void vfs_read(char *path, uint32_t _offset, uint32_t size, char *_buf) {

}

void vfs_write() {

}

void vfs_mkdir() {

}

void vfs_rmdir() {

}

void vfs_mkfile() {

}

void vfs_ls() {

}


void vfs_link() {

}


void vfs_unlink() {

}

void vfs_find() {

}

void vfs_umount() {

}

void vfs_mount() {

}