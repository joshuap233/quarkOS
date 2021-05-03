//
// Created by pjs on 2021/5/1.
//

#ifndef QUARKOS_BUDDY_H
#define QUARKOS_BUDDY_H


struct buddy {
    u8_t total;         // 分配器管理的块个数取log2后加1
    u8_t sizeLog[0];    // 块个数取log2后加1
};

struct buddy *buddy_new(struct buddy *self, u16_t size);

void buddy_free(struct buddy *self, int offset);

u32_t buddy_size(struct buddy *self, int offset);
u16_t buddy_alloc(struct buddy *self, u16_t size);
u32_t buddy_maxBlock();

#define BUDDY_SIZE(size) (2 * (size) * sizeof(u8_t))
#endif //QUARKOS_BUDDY_H
