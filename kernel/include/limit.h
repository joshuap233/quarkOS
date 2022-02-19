//
// Created by pjs on 2022/2/9.
// 为了简化操作系统而进行的一些限制

#ifndef QUARKOS_LIMIT_H
#define QUARKOS_LIMIT_H

#define N_CPU      8
#define DEV_SPACE  0xFE000000
#define DEVSP_SIZE 0x2000000
#define PHYS_TOP   (1*1024*1024*1024 - DEVSP_SIZE)


#endif //QUARKOS_LIMIT_H
