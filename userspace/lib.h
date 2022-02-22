//
// Created by pjs on 2021/6/7.


#ifndef QUARKOS_USERSPACE_LIB_H
#define QUARKOS_USERSPACE_LIB_H

#include <types.h>

#define assert(condition) do{\
    if (!(condition)) {     \
        printf("\nassert error: %s: %s: %u\n",__FILE__,__FUNCTION__,__LINE__); \
        exit(1);                        \
    }\
}while(0)

int fork();

int exit(int errno);

int puts(const char *buf, int32_t len);

int gets(char *buf, int32_t len);

int exec(const char *path);

int cls();

// returns the previous program break
void *sbrk(u32_t size);

void printf(char *__restrict str, ...);

bool memcmp(const void *s1, const void *s2, size_t len);

bool strcmp(const char *s1, const char *s2);

#ifdef TEST
#define test_start   printf("test start: %s\n",__FUNCTION__);
#define test_pass    printf("test pass : %s: %s: %u\n",__FILE__,__FUNCTION__,__LINE__);
#endif // TEST


struct dirent {
    ino_t          d_ino;       /* Inode number */
    off_t          d_off;       /* Not an offset; see below */
    unsigned short d_reclen;    /* Length of this record */
    unsigned char  d_type;      /* Type of file; not supported by all filesystem types */
    char           d_name[256]; /* Null-terminated filename */
};

#endif //QUARKOS_USERSPACE_LIB_H
