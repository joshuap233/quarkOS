//
// Created by pjs on 2021/2/6.
//

#include "link_list.h"
#include "mm.h"
#include "heap.h"

bool list_split(list_t *list, pointer_t va, uint32_t size) {
    //查找空闲列表是否有满足大小的空闲空间并切割
    va = PAGE_ADDR(va);
    pointer_t end = va + size;
    list_t *temp = list, *pre_temp = list;
    while (temp != MM_NULL) {
        if (temp->addr <= va && temp->end_addr >= end) {
            temp->size -= size;
            temp->addr = end;
            if (temp->addr == end) pre_temp->next = temp->next;
            return true;
        }
        pre_temp = temp;
        temp = temp->next;
    }
    return false;
}

void *list_split_ff(list_t *list, uint32_t size) {
    list_t *temp = list, *pre_temp = temp;
    while (temp != MM_NULL) {
        if (temp->size >= size) {
            void *temp_addr = (void *) (temp->addr);
            temp->addr = temp->addr + size;
            temp->size -= size;
            if (temp->addr == temp->end_addr) pre_temp->next = temp->next;
            return temp_addr;
        }
        pre_temp = temp;
        temp = temp->next;
    }
    return MM_NULL;
}


//释放空闲空间
void list_free(list_t *list, pointer_t va, uint32_t size) {
    list_t *temp = list, *pre_temp = temp;
    list_t *new = mallocK(sizeof(list_t));
    new->addr = va;
    new->size = size;
    new->end_addr = va + size;
    new->next = MM_NULL;

    while (temp != MM_NULL && temp->addr <= va) {
        pre_temp = temp;
        temp = temp->next;
    }
    if (temp == pre_temp) {
        //空闲链表第一个节点是静态分配的,不能 free
        list_t t = *list;
        *list = *new;
        *new = t;
        list->next = new;
    } else
        pre_temp->next = new;
    new->next = temp;

    list_merge(list);
}


//合并连续空闲空间
void list_merge(list_t *list) {
    list_t *temp = list->next, *pre_temp = list;
    while (temp != MM_NULL) {
        if (temp->end_addr > pre_temp->addr) {
            pre_temp->end_addr = temp->end_addr;
            pre_temp->size = pre_temp->end_addr - pre_temp->addr;
            pre_temp->next = temp->next;
            freeK(temp);
            temp = pre_temp->next;
        }
        pre_temp = temp;
        temp = temp->next;
    }
}