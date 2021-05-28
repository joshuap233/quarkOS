//
// Created by pjs on 2021/2/1.
//
// TODO: 分配需要分配 3K, 5K, 9K...这种内存依然会造成大量内存碎片
// 可以将多余的内存划分给 slab 分配器?
// 如果多余的内存没有划分给slab,多余的内存是否需要映射到虚拟地址空间?


#include "types.h"
#include "lib/qlib.h"
#include "mm/mm.h"
#include "mm/page_alloc.h"
#include "mm/block_alloc.h"
#include "mm/vm_area.h"

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

static node_t *bst_search(node_t *root, u32_t pn);

static node_t *bst_delete(node_t *root, u32_t pn, node_t **del);

static node_t *bst_insert(node_t *root, node_t *target);

static node_t *bst_deleteMin(node_t *root, node_t **del);

static void bst_print(node_t *root);

uint32_t get_buddy_pn(uint32_t pn, uint8_t sizeLog);

INLINE node_t *new_node();

INLINE void node_free(node_t *node);

/*
 * 物理内存分配器
 * root:        root[11] 管理已分配块,root[0]-root[10] 对应 4K - 4M 内存块
 * allocated:   以分配内存块
 * addr:        物理内存分配器管理的内存起始地址
 * blockSize:   最小内存单元大小
 * listCnt:     list 链表剩余节点数量
 * list:        list 空闲节点链表
 */
struct allocator {
#define MAX_ORDER 10
    node_t *root[MAX_ORDER + 1];
    node_t *allocated;
    node_t *list;
    u16_t listCnt;
    u16_t blockSize;
    uint32_t addr;
} pmm;


void pmm_init() {
    // TODO:还有不足 4M 的内存, 且该管理器管理固定大小内存,而不是所有剩余内存
    // 且 block_alloc 可分配内存块并不连续,但 pmm allocator 只有一个 addr(起始地址)
    pmm.blockSize = PAGE_SIZE;
    u32_t size = block_size() / (4 * K) / 2; //需要分配的节点数
    node_t *list = (node_t *) block_alloc(sizeof(node_t) * size);
    // pmm.add 以下区域为内核
    vm_area_expand(PAGE_ALIGN(block_start()), KERNEL_AREA);

    // 初始化空闲链表
    for (u32_t i = 0; i < size - 1; ++i) {
        LIST_NEXT(list[i]) = &list[i + 1];
    }
    LIST_NEXT(list[size - 1]) = NULL;
    pmm.list = &list[0];
    pmm.listCnt = size;

    // 初始化用与管理 4M 内存块的节点
    for (int i = 0; i < MAX_ORDER; ++i) {
        pmm.root[i] = NULL;
    }

    size = block_size() & (~(u32_t) (4 * M - 1));
    pmm.addr = block_alloc_align(size, PAGE_SIZE);


    pmm.root[MAX_ORDER] = sortedArrayToBST(0, size / (4 * M));
    pmm.allocated = NULL;

#ifdef TEST
    test_alloc();
#endif //TEST
}


ptr_t pm_alloc(u32_t size) {
    assertk(size > 0 && size <= 4 * M);
    node_t *root, *del;
    u32_t pn;
    size = log2(PAGE_ALIGN(size) >> 12);

    for (uint16_t i = size; i <= MAX_ORDER; ++i) {
        root = pmm.root[i];
        if (root) {
            //TODO: 一直删除最小的元素导致树失衡
            pmm.root[i] = bst_deleteMin(root, &del);
            assertk(del);

            del->sizeLog = size;
            pn = del->pn;

            // 被分配内存块插入 allocated
            pmm.allocated = bst_insert(pmm.allocated, del);

            // 不要删 j <= MAX_ORDER
            for (u32_t j = i - 1; j >= size && j <= MAX_ORDER; j--) {
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

u32_t pm_chunk_size(ptr_t addr) {
    assertk(addr >= pmm.addr && (addr & PAGE_MASK) == 0)
    u32_t pn = (addr - pmm.addr) >> 12;
    node_t *node = bst_search(pmm.allocated, pn);
    assertk(node);
    return PAGE_SIZE * SIZE(node->sizeLog);
}

u32_t pm_free(ptr_t addr) {
    assertk(addr >= pmm.addr && (addr & PAGE_MASK) == 0)
    uint16_t i;
    u32_t buddy_pn, pn, retSize;
    node_t *node, *buddy;

    pn = (addr - pmm.addr) >> 12;

    pmm.allocated = bst_delete(pmm.allocated, pn, &node);
    assertk(node);
    retSize = SIZE(node->sizeLog) << 12;

    for (i = node->sizeLog; i < MAX_ORDER; ++i) {
        buddy_pn = get_buddy_pn(pn, node->sizeLog);
        pmm.root[i] = bst_delete(pmm.root[i], buddy_pn, &buddy);
        if (!buddy) break;
        node->sizeLog++;
        pn = MIN(buddy_pn, pn);
        node_free(buddy);
    }
    pmm.root[i] = bst_insert(pmm.root[i], node);

    return retSize;
}

ptr_t pm_alloc_page() {
    return pm_alloc(PAGE_SIZE);
}

uint32_t get_buddy_pn(uint32_t pn, uint8_t sizeLog) {
    size_t size = SIZE(sizeLog);
    return ((pn >> sizeLog) & 1) ? pn - size : pn + size;
}

static node_t *sortedArrayToBST(u16_t start, u16_t size) {
    if (size == 0) return NULL;
    u16_t temp = size;
    size >>= 1;
    u16_t mid = start + size;
    node_t *root = new_node();
    root->pn = mid * (1 << MAX_ORDER);
    root->sizeLog = MAX_ORDER;
    root->left = sortedArrayToBST(start, size);
    root->right = sortedArrayToBST(mid + 1, temp - size - 1);
    return root;
}

static node_t *bst_search(node_t *root, u32_t pn) {
    assertk(root);
    while (root != NULL) {
        if (root->pn == pn) return root;
        root = NEXT_NODE(root, pn);
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

    if (del) *del = *node;
    if (!(*node)->left || !(*node)->right) {
        (*node) = VALID_CHILD(*node);
    } else {
        node_t *min;
        (*node)->right = bst_deleteMin((*node)->right, &min);
        min->right = (*node)->right;
        min->left = (*node)->left;
        *node = min;
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

UNUSED static void bst_print(node_t *root) {
    if (!root) return;
    bst_print(root->left);
    printfk("%u,", ((uint32_t *) root)[0] >> 8);
    bst_print(root->right);
}

INLINE node_t *new_node() {
    assertk(pmm.list != NULL && pmm.listCnt != 0);
    pmm.listCnt--;
    node_t *ret = pmm.list;
    pmm.list = LIST_NEXT(*ret);
    return ret;
}

INLINE void node_free(node_t *node) {
    assertk(node);
    pmm.listCnt++;
    LIST_NEXT(*node) = pmm.list;
    pmm.list = node;
}


#ifdef TEST

size_t cnt = 0;

static void node_cnt(node_t *root) {
    if (!root) return;
    node_cnt(root->left);
    cnt++;
    node_cnt(root->right);
}

static void except_cnt(node_t *root, size_t except) {
    cnt = 0;
    node_cnt(root);
    assertk(cnt == except);
}

static void except_list_cnt(size_t except) {
    size_t count = 0;
    node_t *node = pmm.list;
    while (node) {
        count++;
        node = LIST_NEXT(*node);
    }
    assertk(count == except);
}

ptr_t addr[MAX_ORDER + 1] = {[1 ...MAX_ORDER] = 0};

void test_alloc() {
    test_start
    size_t total, alloc = 0, listCnt = pmm.listCnt;
    except_list_cnt(listCnt);

    // 测试内存块分配
    node_cnt(pmm.root[MAX_ORDER]);
    total = cnt;

    ptr_t temp_adr = pm_alloc(PAGE_SIZE);
    alloc++;
    for (int i = 0; i < MAX_ORDER; ++i)
        except_cnt(pmm.root[i], 1);
    except_cnt(pmm.root[MAX_ORDER], total - 1);
    except_cnt(pmm.allocated, alloc);

    for (int i = 0; i <= MAX_ORDER; ++i) {
        addr[i] = pm_alloc(PAGE_SIZE << i);
        alloc++;
        // 统计 root[0] - root[11] 树节点个数
        for (int j = 0; j <= MAX_ORDER; ++j) {
            if (j == MAX_ORDER) {
                if (i != MAX_ORDER)
                    except_cnt(pmm.root[MAX_ORDER], total - 1);
                else
                    except_cnt(pmm.root[MAX_ORDER], total - 2);
            } else
                except_cnt(pmm.root[j], j <= i ? 0 : 1);
            except_cnt(pmm.allocated, alloc);
        }
    }

    // 测试内存块释放
    for (int i = 0; i <= MAX_ORDER; ++i) {
        pm_free(addr[i]);
        alloc--;

        for (int j = 0; j <= MAX_ORDER; ++j) {
            if (j == MAX_ORDER) {
                if (i != MAX_ORDER)
                    except_cnt(pmm.root[MAX_ORDER], total - 2);
                else
                    except_cnt(pmm.root[MAX_ORDER], total - 1);
            } else
                except_cnt(pmm.root[j], j <= i ? 1 : 0);
            except_cnt(pmm.allocated, alloc);
        }
    }

    pm_free(temp_adr);
    alloc--;
    assertk(alloc == 0);
    for (int i = 0; i < MAX_ORDER; ++i)
        except_cnt(pmm.root[i], 0);
    except_cnt(pmm.root[MAX_ORDER], total);
    except_cnt(pmm.allocated, 0);
    except_list_cnt(listCnt);
    test_pass
}

#endif //TEST