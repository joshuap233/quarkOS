//
// Created by pjs on 2021/4/21.
//

#ifndef QUARKOS_FS_INIT_H
#define QUARKOS_FS_INIT_H

extern void ext2_init();

extern void page_cache_init();

extern void vfs_init();

#ifdef TEST

extern void test_ide_rw();

extern void test_ext2();

extern void test_dma_rw();

extern void test_vfs();

#endif // TEST

#endif //QUARKOS_FS_INIT_H
