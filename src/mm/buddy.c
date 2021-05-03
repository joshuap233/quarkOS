//
// Created by pjs on 2021/4/29.
//

#include "types.h"
#include "lib/qlib.h"
#include "mm/buddy.h"

#define LEFT_LEAF(index) ((index) * 2 + 1)
#define RIGHT_LEAF(index) ((index) * 2 + 2)
#define PARENT(index) ( ((index) + 1) / 2 - 1)

#define IS_POWER_OF_2(x) (!((x)&((x)-1)))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define SIZE(x) (((x)==1) ?1: (2<<((x)-2)))
#define LOG(x) (log_(x) +1)


static u8_t log_(u16_t size) {
    if (size == 0) return 0;
    u8_t cnt;
    for (cnt = 0; size != 1; cnt++)
        size >>= 1;
    return cnt;
}

// size 为分配器管理的内存块个数
struct buddy *buddy_new(struct buddy *self, u16_t size) {
    assertk(size > 1 && IS_POWER_OF_2(size));

    self->total = LOG(size);
    u16_t node_size = self->total + 1;

    for (u32_t i = 0; i < 2 * size - 1; ++i) {
        if (IS_POWER_OF_2(i + 1))
            node_size--;
        self->sizeLog[i] = node_size;
    }

    return self;
}


// 返回分配的内存在需要管理的内存块中偏移
u16_t buddy_alloc(struct buddy *self, u16_t size) {
    assertk(self);
    u16_t index = 0, offset, sizeLog;

    size = (size != 0 && IS_POWER_OF_2(size)) ? LOG(size) : LOG(size) + 1;

    if (self->sizeLog[index] < size)
        return -1;

    for (sizeLog = self->total; sizeLog != size; sizeLog--) {
        if (self->sizeLog[LEFT_LEAF(index)] >= size)
            index = LEFT_LEAF(index);
        else
            index = RIGHT_LEAF(index);
    }

    self->sizeLog[index] = 0;
    offset = (index + 1) * SIZE(sizeLog) - SIZE(self->total);

    while (index) {
        index = PARENT(index);
        self->sizeLog[index] = MAX(self->sizeLog[LEFT_LEAF(index)], self->sizeLog[RIGHT_LEAF(index)]);
    }

    return offset;
}

void buddy_free(struct buddy *self, int offset) {
    u16_t node_size = 1, size = SIZE(self->total), index = offset + size - 1;
    u16_t lNode, rNode;

    assertk(self && offset >= 0 && offset < size);

    for (; self->sizeLog[index]; index = PARENT(index)) {
        node_size++;
        if (index == 0) return;
    }

    self->sizeLog[index] = node_size;

    while (index) {
        index = PARENT(index);
        node_size++;

        lNode = self->sizeLog[LEFT_LEAF(index)];
        rNode = self->sizeLog[RIGHT_LEAF(index)];

        if (lNode + rNode == node_size)
            self->sizeLog[index] = node_size;
        else
            self->sizeLog[index] = MAX(lNode, rNode);
    }
}

u32_t buddy_size(struct buddy *self, int offset) {
    u16_t node_size = 1, size = SIZE(self->total);

    assertk(self && offset >= 0 && offset < size);

    for (u16_t i = offset + size - 1; self->sizeLog[i]; i = PARENT(i))
        node_size++;

    return SIZE(node_size);
}

// 返回分配器最大可分配内存块大小
u32_t buddy_maxBlock(struct buddy *self) {
    return SIZE(self->sizeLog[0]);
}
