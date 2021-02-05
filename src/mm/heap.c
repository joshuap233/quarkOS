//
// Created by pjs on 2021/2/3.
//

#include "heap.h"
#include "types.h"
#include "mm.h"
#include "virtual_mm.h"
//内核堆
#define HEAP_NULL (void*)0

//追踪空闲空间
typedef struct link_list {
    pointer_t *addr;
    uint32_t length;
    struct link_list *next;
} link_list_t;

static link_list_t heap;


//合并空闲空间
static void merge() {

}

void heap_init() {
    heap.length = PAGE_SIZE;
    heap.addr = vmm_alloc(1);
    heap.next = HEAP_NULL;
}

pointer_t *mallocK(size_t size) {
    link_list_t *temp = &heap;

    while (true) {
        if (temp->length > size) {
            pointer_t *res = temp->addr;
            temp->addr += size;
            temp->length -= size;
            return res;
        }
        if (temp->next == HEAP_NULL) {

        }
        temp->next = temp;
    }

}



// mallocK
// freeK

//扩展堆区域

