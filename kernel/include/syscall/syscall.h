//
// Created by pjs on 2021/6/4.
//

#ifndef QUARKOS_SYSCALL_H
#define QUARKOS_SYSCALL_H

#define SYS_CALL_NO  0x80

#define SYS_EXEC     0
#define SYS_GETCHAR  1
#define SYS_PUTCHAR  2
#define SYS_MKDIR    3
#define SYS_MKFILE   4
#define SYS_RMDIR    5
#define SYS_UNLINK   6
#define SYS_OPEN     7
#define SYS_CLOSE    8
#define SYS_FORK     9
#define SYS_READ     10
#define SYS_WRITE    11
#define SYS_SLEEP    12
#define SYS_EXIT     13
#define SYS_CLS      14
#define SYS_GETCWD   15
#define SYS_SBRK     16

#endif //QUARKOS_SYSCALL_H
