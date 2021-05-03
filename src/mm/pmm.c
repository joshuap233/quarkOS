//
// Created by pjs on 2021/2/1.
//
// 使用栈结构管理物理内存
#include "types.h"
#include "lib/qlib.h"
#include "mm/mm.h"
#include "mm/pmm.h"
#include "multiboot2.h"
#include "lib/list.h"
#include "mm/buddy.h"

#define entry(ptr) list_entry(ptr,struct block,node)


typedef struct treeNode {
    u16_t idx;
    struct treeNode *left;
    struct treeNode *right;
    struct buddy buddy[0];
} node_t;

#define VALID_CHILD(node) ((node)->left ? (node)->left : (node)->right)
#define NEXT_NODE(node, v) (((node)->idx > (v)) ? (node)->left : (node)->right)
#define NEXT_NODE_ADDR(node, v) ((node)->idx > (v) ? &(node)->left : &(node)->right)

static node_t *sortedArrayToBST(u16_t start, u16_t size);

static node_t *bst_search(node_t *node, u16_t idx);

static node_t *bst_delete(node_t *root, u16_t key);

static node_t *bst_insert(node_t *root, node_t *target);

/*
 * 物理内存分配器,管理多个 buddy 分配器
 * root: 管理可分配4K-4M内存块的buddy分配器,root[0]管理已经完全分配的buddy分配器
 *       buddy分配器根节点值为可分配最大内存块(2的幂),而不是该分配器可用内存
 * node     :  buddy 分配器列表
 * cnt      :  buddy 分配器个数
 * blockSize:  buddy 分配器基本内存单元大小(4K的整数倍)
 * buddySize:  一个 buddy 分配器管理的内存大小
 * addr     :  物理内存分配器管理的内存起始地址
 */
struct allocator {
#define MAX_ORDER 12
    node_t *root[MAX_ORDER + 1];
    node_t *node;
    u16_t cnt;
    u16_t blockSize;
    u32_t buddySize;
    uint32_t addr;
} pm;

void pm_init() {
    //TODO:不足 4M 的内存分个 slab
    pm.buddySize = 4 * M;
    pm.blockSize = PAGE_SIZE;
    pm.cnt = DIV_CEIL(bInfo.mem_total, pm.buddySize);
    u32_t bSize = (sizeof(node_t) + BUDDY_SIZE(MAX_ORDER));

    pm.node = (node_t *) split_mmap(pm.cnt * bSize);

    for (u16_t i = 0; i < MAX_ORDER; ++i) {
        pm.root[i] = NULL;
    }
    pm.root[MAX_ORDER] = sortedArrayToBST(0, pm.cnt);

    //TODO: 以及被 split_mmap 的内存?
    for (u16_t i = 0; i < pm.cnt; ++i) {
        buddy_new(pm.node[i].buddy, pm.buddySize / pm.buddySize);
    }
#ifdef TEST
    test_physical_mm();
#endif //TEST
}


ptr_t pm_alloc(size_t size) {
    size = PAGE_ALIGN(size);
}

void pm_free(ptr_t addr) {
    // TODO: 释放的时候需要使用 addr 算出buddy需要的偏移
    // 并且 使用 addr 算出 addr 属于哪个 buddy 分配器???
}

ptr_t pm_alloc_page() {
    for (int i = 1; i < MAX_ORDER; ++i) {
        if (pm.root[i]) {
            u16_t offset = buddy_alloc(pm.root[i]->buddy, pm.blockSize / PAGE_SIZE);
            pm.root[i] = bst_delete(pm.root[i], pm.root[i]->idx);
            bst_insert(pm.root[i - 1], pm.root[i]);
            return pm.addr + pm.buddySize * pm.root[i]->idx + offset * pm.blockSize;
        }
    }
    return PMM_NULL;
}

static node_t *sortedArrayToBST(u16_t start, u16_t size) {
    if (size == 0) return NULL;
    u16_t temp = size;
    size >>= 1;
    u16_t mid = start + size;
    node_t *root = &pm.node[mid];
    root->idx = mid;
    root->left = sortedArrayToBST(start, size);
    root->right = sortedArrayToBST(mid + 1, temp - size - 1);
    return root;
}

static node_t *bst_search(node_t *node, u16_t idx) {
    assertk(node);
    while (node != NULL) {
        if (node->idx == idx) return node;
        node = NEXT_NODE(node, idx);
    }
    return NULL;
}


static node_t *bst_del_right_min(node_t *root) {
    node_t **node = &root->right, *temp;
    while ((*node)->left != NULL)
        node = &(*node)->left;
    temp = *node;
    *node = (*node)->right;
    return temp;
}

// 返回新根
static node_t *bst_delete(node_t *root, u16_t key) {
    node_t **node = &root;
    while (*node != NULL && key != (*node)->idx)
        node = NEXT_NODE_ADDR(*node, key);

    assertk(*node);
//    if (*node == NULL) return root;

    if (!(*node)->left || !(*node)->right) {
        (*node) = VALID_CHILD(*node);
    } else {
        node_t *min = bst_del_right_min(*node);
        (*node)->idx = min->idx;
    }
    return root;
}

static node_t *bst_insert(node_t *root, node_t *target) {
    assertk(root && target);
    node_t **node = &root;
    while (*node != NULL)
        node = NEXT_NODE_ADDR(*node, target->idx);
    target->left = NULL;
    target->right = NULL;
    *node = target;
    return root;
}

#ifdef TEST

void test_physical_mm() {

}

#endif //TEST