//
// Created by pjs on 2021/4/21.
//

#ifndef QUARKOS_INIT_H
#define QUARKOS_INIT_H

void ext2_init();

void page_cache_init();


#ifdef TEST

void test_ide_rw();

void test_ext2_fs();

void test_dma_rw();

#endif // TEST

#endif //QUARKOS_INIT_H
