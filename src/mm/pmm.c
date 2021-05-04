//
// Created by pjs on 2021/2/1.
//
// 使用栈结构管理物理内存
#include "types.h"
#include "lib/qlib.h"
#include "mm/mm.h"
#include "mm/pmm.h"
#include "multiboot2.h"


typedef struct treeNode {
#define SIZE(sizeLog) (1<<(sizeLog)) // 获取实际内存单元个数
    u32_t sizeLog: 8;                // 当前节点管理的内存单元(4K)个数取log2
    u32_t pn: 24;                    // 页帧号
    struct treeNode *left;
    struct treeNode *right;
} node_t;


#define VALID_CHILD(node) ((node)->left ? (node)->left : (node)->right)
#define NEXT_NODE(node, v) (((node)->pn > (v)) ? (node)->left : (node)->right)
#define NEXT_NODE_ADDR(node, v) ((node)->pn > (v) ? &(node)->left : &(node)->right)
#define LIST_NEXT(node) ((node).right)

static node_t *sortedArrayToBST(u16_t start, u16_t size);

static node_t *bst_search(node_t *node, u32_t pn);

static node_t *bst_delete(node_t *root, u32_t pn, node_t **del);

static node_t *bst_insert(node_t *root, node_t *target);

static node_t *bst_deleteMin(node_t *root, node_t **del);

static void bst_print(node_t *root);

static u8_t log2(uint16_t val);

INLINE uint32_t get_buddy_pn(uint32_t pn, uint8_t sizeLog);

INLINE node_t *new_node();

INLINE void list_free(node_t *node);

/*
 * 物理内存分配器
 * root:        root[11] 管理已分配块,root[0]-root[10] 对应 4K - 4M 内存块
 * allocated:   以分配内存块
 * addr:        物理内存分配器管理的内存起始地址
 * blockSize:   最小内存单元大小
 * list:        list 空闲节点链表
 */
struct allocator {
#define MAX_ORDER 10
    node_t *root[MAX_ORDER + 1];
    node_t *allocated;
    node_t *list;
    uint32_t addr;
    u16_t blockSize;
} pmm;


void pm_init() {
    // TODO:不足 4M 的内存用于 slab
    pmm.blockSize = PAGE_SIZE;
    u16_t size = bInfo.mem_total / (4 * K) / 2;//需要分配的节点数
    node_t *list = (node_t *) split_mmap(sizeof(node_t) * size);

    // 初始化空闲链表
    for (int i = 0; i < size - 1; ++i) {
        LIST_NEXT(list[i]) = &list[i + 1];
    }
    LIST_NEXT(list[size - 1]) = NULL;
    pmm.list = &list[0];

    // 初始化用与管理 4M 内存块的节点
    for (int i = 0; i < MAX_ORDER; ++i) {
        pmm.root[i] = NULL;
    }
    pmm.addr = PAGE_ALIGN(bInfo.vmm_start);
    uint16_t rSize = (bInfo.mem_total - (pmm.addr - bInfo.vmm_start)) / (4 * M);
    pmm.root[MAX_ORDER] = sortedArrayToBST(0, rSize);
    pmm.allocated = NULL;
#ifdef TEST
    test_physical_mm();
#endif //TEST
}


ptr_t pm_alloc(int32_t size) {
    assertk(size > 0 && size < 4 * M);
    size = log2(PAGE_ALIGN(size) >> 12);

    for (uint16_t i = size; i <= MAX_ORDER; ++i) {
        node_t *root = pmm.root[i];
        if (root) {
            node_t *del;
            pmm.root[i] = bst_deleteMin(root, &del);
            del->sizeLog = size;
            // 被分配内存块插入 allocated
            pmm.allocated = bst_insert(pmm.allocated, del);

            u32_t pn = del->pn;
            for (int32_t j = i - 1; j >= size; j--) {
                node_t *new = new_node();
                new->sizeLog = j;
                new->pn = pn + SIZE(j + 1) / 2;
                pmm.root[j] = bst_insert(pmm.root[j], new);
            }
            return pmm.addr + (pn << 12);
        }
    }
    return PMM_NULL;
}

void pm_free(ptr_t addr) {
    assertk(addr > pmm.addr && (addr & PAGE_MASK) == 0)
    u32_t pn = (addr - pmm.addr) >> 12;
    node_t *node, *buddy;
    pmm.allocated = bst_delete(pmm.allocated, pn, &node);
    assertk(node);

    uint32_t buddy_pn = get_buddy_pn(pn, node->sizeLog);
    for (uint16_t i = node->sizeLog; i <= MAX_ORDER; ++i) {
        pmm.root[i] = bst_delete(pmm.root[i], buddy_pn, &buddy);
        if (!buddy) {
            pmm.root[i] = bst_insert(pmm.root[i], node);
            break;
        }
    }
}

ptr_t pm_alloc_page() {
    return pm_alloc(PAGE_SIZE);
}

INLINE uint32_t get_buddy_pn(uint32_t pn, uint8_t sizeLog) {
    size_t size = SIZE(sizeLog);
    return ((pn >> sizeLog) & 0x3) ? pn - size : pn + size;
}

static node_t *sortedArrayToBST(u16_t start, u16_t size) {
    if (size == 0) return NULL;
    u16_t temp = size;
    size >>= 1;
    u16_t mid = start + size;
    node_t *root = new_node();
    root->pn = mid * (1 << MAX_ORDER);
    root->pn = mid;
    root->left = sortedArrayToBST(start, size);
    root->right = sortedArrayToBST(mid + 1, temp - size - 1);
    return root;
}

static node_t *bst_search(node_t *node, u32_t pn) {
    assertk(node);
    while (node != NULL) {
        if (node->pn == pn) return node;
        node = NEXT_NODE(node, pn);
    }
    return NULL;
}


// 返回新根
static node_t *bst_delete(node_t *root, u32_t pn, node_t **del) {
    node_t **node = &root;
    while (*node != NULL && pn != (*node)->pn)
        node = NEXT_NODE_ADDR(*node, pn);

    if (*node == NULL) {
        if (del)*del = NULL;
        return root;
    }

    if (!(*node)->left || !(*node)->right) {
        if (del)*del = *node;
        (*node) = VALID_CHILD(*node);
    } else {
        node_t *min;
        (*node)->right = bst_deleteMin((*node)->right, &min);
        if (del)*del = min;
        (*node)->pn = min->pn;
    }
    return root;
}


static node_t *bst_deleteMin(node_t *root, node_t **del) {
    assertk(root);
    node_t **node = &root;
    while ((*node)->left != NULL)
        node = &(*node)->left;
    if (del)*del = *node;
    (*node) = (*node)->right;
    return root;
}

static node_t *bst_insert(node_t *root, node_t *target) {
    assertk(target);
    target->left = NULL;
    target->right = NULL;
    if (!root) return target;

    node_t **node = &root;
    while (*node != NULL)
        node = NEXT_NODE_ADDR(*node, target->pn);
    *node = target;
    return root;
}

static void bst_print(node_t *root) {
    if (!root) return;
    bst_print(root->left);
    printfk("%u,", ((uint32_t *) root)[0] >> 8);
    bst_print(root->right);
}

INLINE node_t *new_node() {
    assertk(pmm.list != NULL);
    node_t *ret = pmm.list;
    pmm.list = LIST_NEXT(*ret);
    return ret;
}

INLINE void list_free(node_t *node) {
    assertk(node);
    LIST_NEXT(*node) = pmm.list;
    if (pmm.list == NULL) {
        pmm.list = node;
    }
}

static u8_t log2(uint16_t val) {
    u8_t cnt = 0;
    while (val != 1) {
        cnt++;
        val >>= 1;
    }
    return cnt;
}

#ifdef TEST

void test_physical_mm() {

}

#endif //TEST