//
// Created by pjs on 2021/2/6.
//
// 用于管理空闲虚拟内存

#include "free_list.h"
#include "mm.h"
#include "heap.h"

static free_list_t free_list[LIST_COUNT];
static free_list_t *header;
//静态分配一页的内存用于空闲链表,
//需要更多空闲链表使,动态分配链表(堆)
//栈满时,释放动态分配的空闲链表
static stack_t stack = {
        .top = 0,
        .size = STACK_SIZE
};

static inline void push(free_list_t *addr) {
    stack.list[stack.top++] = (pointer_t) addr;
}

static inline pointer_t pop() {
    return stack.list[--stack.top];
}

//分配一个链表节点
static free_list_t *list_alloc() {
    if (stack.top == stack.size) {
        return mallocK(sizeof(free_list_t));
    }
    return (free_list_t *) pop();
}

// 销毁一个链表节点
static void list_destroy(free_list_t *node) {
    if (stack.top == stack.size) {
        //栈满则释放栈内动态分配的链表
        uint32_t count = 0;
        free_list_t *start = &free_list[0], *end = &free_list[LIST_COUNT - 1];
        //循环结束可以空出至少 STACK_SIZE-LIST_COUNT 个节点,且不会被经常调用,理论上不会有性能问题 ?
        for (uint32_t i = 0; i < stack.size; ++i) {
            free_list_t *item = (free_list_t *) stack.list[i];
            if (item < start || item > end) {
                stack.list[i] = 0;
                count++;
                freeK(item);
            } else {
                stack.list[i - count] = stack.list[i];
            }
        }
    }
    push(node);
}

//size 为内核以下占用的虚拟内存
void free_list_init(uint32_t size) {
    free_list_t *list = list_alloc();
    list->addr = size;
    list->next = MM_NULL;
    list->prev = MM_NULL;
    list->size = PHYMM - size;
    header = list;
}


bool list_split(pointer_t va, uint32_t size) {
    //查找空闲列表是否有满足大小的空闲空间并切割
    va = PAGE_ADDR(va);
    pointer_t end = va + size;
    free_list_t *temp = header;
    while (temp != MM_NULL) {
        if (temp->addr <= va && (temp->addr + temp->size) >= end) {
            temp->size -= size;
            temp->addr = end;
            if (temp->size == 0) {
                if (temp->prev != MM_NULL)
                    temp->prev->next = temp->next;
                else
                    header = temp->next;
                list_destroy(temp);
            }
            return true;
        }
        temp = temp->next;
    }
    return false;
}

//first fit
void *list_split_ff(uint32_t size) {
    free_list_t *temp = header;
    while (temp != MM_NULL) {
        if (temp->size >= size) {
            void *temp_addr = (void *) temp->addr;
            if (temp->size == size) {
                if (temp->prev != MM_NULL)
                    temp->prev->next = temp->next;
                else
                    header = temp->next;
                list_destroy(temp);
            } else {
                temp->addr = temp->addr + size;
                temp->size -= size;
            }
            return temp_addr;
        }
        temp = temp->next;
    }
    return MM_NULL;
}

//合并连续空闲空间
//alloc 为合并前插入的链表节点
static void list_merge(free_list_t *alloc) {
    free_list_t *h = alloc, *t = alloc;
    //向前合并
    while (h->prev != MM_NULL && (h->prev->addr + h->prev->size >= h->addr)) {
        h = h->prev;
        if (h->next != alloc) list_destroy(h->next);
    }
    //向后合并
    while (t->next != MM_NULL && (t->addr + t->size >= t->next->addr)) {
        t = t->next;
        if (t->prev != alloc) list_destroy(t->prev);

    }
    bool equal = false;
    if (h != t) {
        equal = (t == alloc) || (h == alloc);
        h->next = t->next;
        h->size = t->addr - h->addr;
        list_destroy(t);
    }
    if (!equal) list_destroy(alloc);
}


//释放空闲空间
void list_free(pointer_t va, uint32_t size) {
    free_list_t *temp = header;
    free_list_t *new = list_alloc();
    new->addr = va;
    new->size = size;
    new->next = MM_NULL;

    while (temp != MM_NULL && temp->addr <= va)
        temp = temp->next;

    if (temp == header) {
        new->prev = MM_NULL;
        header = new;
    } else {
        new->prev = temp->prev;
        temp->prev->next = new;
    }

    new->next = temp;
    list_merge(new);
}
