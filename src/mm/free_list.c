//
// Created by pjs on 2021/2/6.
//

//freelist(循环双向链表)用于管理空闲虚拟内存
//静态分配一页的内存用于空闲链表,使用栈管理链表节点
//需要更多链表节点时,动态分配链表(堆)
//栈满时, 释放动态分配的空闲链表节点

#include "mm/free_list.h"
#include "mm/mm.h"
#include "mm/heap.h"
#include "klib/list.h"

#define fl_entry(ptr) list_entry(ptr, free_list_t, head)
#define HEAD vmm_list.header
#define NXT_ENTRY(ptr) fl_entry((ptr)->next)
#define PRV_ENTRY(ptr) fl_entry((ptr)->prev)

static free_list_t free_list[LIST_COUNT];
static vmm_list_t vmm_list;
static free_stack_t stack;

__attribute__((always_inline))
static inline void push(free_list_t *addr) {
    stack.list[stack.top++] = addr;
}

__attribute__((always_inline))
static inline free_list_t *pop() {
    return stack.list[--stack.top];
}


//分配一个链表节点
__attribute__((always_inline))
static inline free_list_t *list_alloc() {
    if (STACK_EMPTY(stack)) {
        return mallocK(sizeof(free_list_t));
    }
    return (free_list_t *) pop();
}



// 销毁一个链表节点
static void list_destroy(free_list_t *node) {
    if (STACK_FULL(stack)) {
        //栈满则释放栈内动态分配的链表
        uint32_t count = 0;
        free_list_t *start = &free_list[0], *end = &free_list[LIST_COUNT - 1];
        //循环结束可以空出至少 STACK_SIZE-LIST_COUNT 个节点,且不会被经常调用,理论上不会有性能问题 ??
        for (uint32_t i = 0; i < stack.size; ++i) {
            free_list_t *dyna_list = (free_list_t *) stack.list[i];
            if (dyna_list < start || dyna_list > end) {
                count++;
                freeK(dyna_list);
            } else {
                stack.list[i - count] = stack.list[i];
            }
        }
        stack.top -= count;
    }
    push(node);
}

static void stack_init() {
    stack.top = 0;
    stack.size = STACK_SIZE;

    for (int i = 0; i < LIST_COUNT; ++i) {
        push(&free_list[i]);
    }
}

//size 为内核以下占用的虚拟内存
void free_list_init(uint32_t size) {
    size = SIZE_ALIGN(size);

    list_header_init(&HEAD);
    stack_init();

    free_list_t *list = list_alloc();
    list->addr = size;
    list_add_next(&list->head, &HEAD);
    list->size = PHYMM - size;
    vmm_list.size = list->size;

    test_list_stack();
    test_list_mem_split();
    test_list_split_ff();
    test_list_split();
}

static void list_mem_split(pointer_t va, uint32_t size, free_list_t *node) {
    pointer_t va_end_n = va + size;
    pointer_t list_end_n = node->size + node->addr;
    if (node->size == size) {
        list_del(&node->head);
        list_destroy(node);
    } else if (node->addr == va) {
        node->addr = va_end_n;
        node->size -= size;
    } else if (va_end_n == list_end_n) {
        node->size -= size;
    } else {
        node->size = va - node->addr;
        free_list_t *new = list_alloc();
        list_add_next(&new->head, &node->head);
        new->addr = va_end_n;
        new->size = list_end_n - va_end_n;
    }
    vmm_list.size -= size;
}

//查找空闲列表是否有满足大小的空闲空间并切割
bool list_split(pointer_t va, uint32_t size) {
    size = SIZE_ALIGN(size);
    va = PAGE_ADDR(va);
    if (vmm_list.size < size) return false;

    list_for_each(&HEAD) {
        free_list_t *tmp = fl_entry(hdr);
        // 各 -1, 防止溢出
        if ((tmp->addr <= va) && (tmp->addr - 1 + tmp->size >= va - 1 + size)) {
            list_mem_split(va, size, tmp);
            return true;
        }
    }
    return false;
}

//first fit
void *list_split_ff(uint32_t size) {
    size = SIZE_ALIGN(size);
    if (vmm_list.size < size) return false;
    list_for_each(&HEAD) {
        free_list_t *tmp = fl_entry(hdr);
        if (tmp->size >= size) {
            void *ret = (void *) tmp->addr;
            list_mem_split(tmp->addr, size, tmp);
            return ret;
        }
    }
    return MM_NULL;
}

//合并连续空闲空间
//alloc 为合并前插入的链表节点
static void list_merge(free_list_t *alloc) {
    list_head_t *h = &alloc->head, *t = alloc->head.next;
    size_t size = 0;

    while (h->prev != &HEAD && (PRV_ENTRY(h)->addr + PRV_ENTRY(h)->size >= fl_entry(h)->addr)) {
        size += fl_entry(h)->size;
        h = h->prev;
    }

    while (t != &HEAD && (PRV_ENTRY(h)->addr + PRV_ENTRY(h)->size >= fl_entry(t)->addr)) {
        size += fl_entry(t)->size;
        t = t->next;
    }

    if (h->next != t) {
        list_head_t *hdr = h->next;
        fl_entry(h)->size += size;
        while (hdr != t) {
            hdr = hdr->next;
            list_destroy(PRV_ENTRY(hdr));
        }
        list_link(h,t);
    }
}


//释放虚拟内存
void list_free(pointer_t va, uint32_t size) {
    list_head_t *hdr = HEAD.next;
    free_list_t *new = list_alloc();
    new->addr = va;
    new->size = size;

    while (hdr != &HEAD && fl_entry(hdr)->addr <= va)
        hdr = hdr->next;

    list_add_prev(&new->head, hdr);
    vmm_list.size += size;
    list_merge(new);
}

//=============== 测试 ================

void test_list_stack() {
    test_start;
    free_list_t *addr[3];
    size_t size = stack.top;
    addr[0] = list_alloc();
    assertk(addr[0] != NULL);
    assertk(stack.top == size - 1);
    list_destroy(addr[0]);
    assertk(stack.top == size);

    addr[1] = list_alloc();
    assertk(addr[1] == addr[0]);
    test_pass;
}

//需要在堆初始化后调用
void test_list_stack2() {
    test_start;
    size_t size = STACK_SIZE + 6;
    free_list_t *list[size];
    uint32_t top = stack.top;
    for (uint32_t i = 0; i < size; i++) {
        list[i] = list_alloc();
        assertk(list[i] != NULL);
    }
    for (int i = size - 1; i >= 0; --i) {
        list_destroy(list[i]);
    }
    assertk(top == stack.top);
    test_pass;
}

void test_list_mem_split() {
    test_start;
    list_head_t *hdr = HEAD.next;
    size_t total = vmm_list.size;
    assertk(hdr->next->next == hdr);
    pointer_t addr[3] = {fl_entry(hdr)->addr, fl_entry(hdr)->addr + 3 * PAGE_SIZE, fl_entry(hdr)->addr + fl_entry(hdr)->size - PAGE_SIZE};
    list_mem_split(addr[0], PAGE_SIZE, fl_entry(hdr));
    list_mem_split(addr[2], PAGE_SIZE, fl_entry(hdr));
    list_mem_split(addr[1], PAGE_SIZE, fl_entry(hdr));
    assertk(hdr->next->next->next == hdr);
    list_free(addr[0], PAGE_SIZE);
    list_free(addr[1], PAGE_SIZE);
    list_free(addr[2], PAGE_SIZE);
    assertk(HEAD.next->next == &HEAD);
    assertk(fl_entry(HEAD.next)->size == total);
    assertk(vmm_list.size == total);
    test_pass;
}


void test_list_split_ff() {
    test_start;
    void *addr[3];
    size_t total = vmm_list.size;
    size_t size = total - PAGE_SIZE * 3;
    addr[0] = list_split_ff(PAGE_SIZE);
    addr[1] = list_split_ff(PAGE_SIZE);
    addr[2] = list_split_ff(PAGE_SIZE);
    list_for_each(&HEAD) {
        size -= fl_entry(hdr)->size;
    }
    assertk(size == 0);

    list_free((pointer_t) addr[0], PAGE_SIZE);
    list_free((pointer_t) addr[1], PAGE_SIZE);
    list_free((pointer_t) addr[2], PAGE_SIZE);
    assertk(vmm_list.size == fl_entry(HEAD.next)->size);
    assertk(HEAD.next->next == &HEAD);
    test_pass;
}

void test_list_split() {
    test_start;
    list_head_t *h = HEAD.next;
    pointer_t addr[3] = {
            fl_entry(h)->addr, fl_entry(h)->addr + 3 * PAGE_SIZE, fl_entry(h)->addr + fl_entry(h)->size - PAGE_SIZE
    };
    size_t total = vmm_list.size;
    size_t size = total - PAGE_SIZE * 3;
    assertk(list_split(addr[0], PAGE_SIZE));
    assertk(list_split(addr[1], PAGE_SIZE));
    assertk(list_split(addr[2], PAGE_SIZE));
    list_for_each(&HEAD) {
        size -= fl_entry(hdr)->size;
    }
    assertk(size == 0);

    list_free((pointer_t) addr[0], PAGE_SIZE);
    list_free((pointer_t) addr[1], PAGE_SIZE);
    list_free((pointer_t) addr[2], PAGE_SIZE);
    assertk(vmm_list.size == fl_entry(HEAD.next)->size);
    assertk(vmm_list.size == total);
    assertk(HEAD.next->next == &HEAD);
    test_pass;
}

