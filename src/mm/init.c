//
// Created by pjs on 2021/2/1.
//

#include "mm/free_list.h"
#include "mm/init.h"

void mm_init() {
    phymm_init();
    vmm_init();
    heap_init();

#ifdef TEST
    test_list_stack2();
#endif //TEST
}
