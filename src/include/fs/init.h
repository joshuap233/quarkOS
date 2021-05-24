//
// Created by pjs on 2021/4/21.
//

#ifndef QUARKOS_FS_INIT_H
#define QUARKOS_FS_INIT_H

void ext2_init();

void page_cache_init();

void vfs_init();

#ifdef TEST

void test_ide_rw();

void test_ext2();

void test_dma_rw();

void test_vfs();

#endif // TEST

#endif //QUARKOS_FS_INIT_H
