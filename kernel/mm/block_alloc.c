//
// Created by pjs on 2021/5/5.
//
// 最原始的物理内存管理器
#include <mm/block_alloc.h>
#include <multiboot2.h>
#include <lib/qlib.h>
#include <mm/mm.h>
#include <lib/qstring.h>

#define block_entry block_mem_entry

// (g_mem_start+HIGH_MEM) - kernel_end 之间的内存为
// 内核初始化消耗的内存,需要在初始化页表时直接映射
ptr_t g_mem_start = 0;

block_allocator_t blockAllocator;

static void sort_block_info();

static void reload();

// 直接将块信息头嵌入内存会导致开启分页后无法使用
void memBlock_init() {
    blockAllocator.size = 0;
    list_header_init(&blockAllocator.head);

    u32_t cnt = 0;
    boot_mmap_entry_t *entry, *firstMem = NULL;

    // 统计可用内存区域个数
    for_each_mmap(entry) {
        assertk(entry->zero == 0);
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
            //移除内核以下可用内存
            if (entry->addr < K_END) {
                uint64_t end_addr = entry->addr + entry->len;
                if (end_addr < K_END) {
                    entry->type = MULTIBOOT_MEMORY_RESERVED;
                    continue;
                }
                entry->addr = K_END;
                entry->len = end_addr - entry->addr;
            }
            if (!firstMem || firstMem->addr > entry->addr) {
                firstMem = entry;
            }
            cnt++;
        }
    }

    // 区域必须在临时页表内映射区域内
    assertk(firstMem->addr < 4 * M);
    assertk(cnt != 0);

    // 初始化 block 分配器
    u32_t alloc = sizeof(struct blockInfo) * cnt;
    struct blockInfo *infos = (void *) firstMem->addr + HIGH_MEM;
    assertk(firstMem != NULL && firstMem->len > alloc);
    firstMem->len -= alloc;
    firstMem->addr += alloc;

    struct blockInfo *info = infos;
    for_each_mmap(entry) {
        assertk(entry->zero == 0);
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
            blockAllocator.size += entry->len;
            info->addr = entry->addr;
            info->size = entry->len;
            list_add_prev(&info->head, &blockAllocator.head);
            info++;
        }
    }
    assertk(info == (void *) infos + alloc);

    // 内存块按地址排序
    sort_block_info();
    blockAllocator.addr = block_entry(blockAllocator.head.next)->addr;
    reload();
}


static void sort_block_info() {
    // blockInfo_t 按地址大小排序
    list_head_t *hdr, *next;
    list_for_each_del(hdr, next, &blockAllocator.head) {
        list_head_t *tmp = hdr->prev;
        while (tmp != &blockAllocator.head &&
               block_entry(hdr) < block_entry(tmp)) {
            tmp = tmp->prev;
        }

        if (tmp != hdr->prev) {
            list_del(hdr);
            list_add_next(hdr, tmp);
        }
    }
}

INLINE ptr_t block_align_size(ptr_t addr, ptr_t size) {
    return PAGE_ADDR(size - (PAGE_ALIGN(addr) - addr));
}


ptr_t block_alloc(u32_t size) {
    assertk(blockAllocator.size >= size);
    size = MEM_ALIGN(size, 4);
    list_head_t *hdr;
    blockInfo_t *info;
    ptr_t addr = 0;
    list_for_each(hdr, &blockAllocator.head) {
        info = block_entry(hdr);
        if (info->size >= size) {
            addr = info->addr;
            info->addr += size;
            info->size -= size;

            blockAllocator.size -= size;
            if (&info->head == blockAllocator.head.next) {
                blockAllocator.addr = info->addr;
            }
            break;
        }
    }
    assertk(addr != 0);

    return addr;
}

ptr_t block_alloc_align(u32_t size, u32_t align) {
    assertk(align > 4 && (align & 0x3) == 0);
    ptr_t addr = block_alloc(size + align - 1);
    return MEM_ALIGN(addr, align);
}

u32_t block_start() {
    return blockAllocator.addr;
}

u32_t block_end() {
    assertk(!list_empty(&blockAllocator.head));
    blockInfo_t *info = block_mem_entry(blockAllocator.head.prev);
    return info->addr + info->size;
}

u32_t block_size() {
    return blockAllocator.size;
}


//移动 信息块
static void *move(void *addr, size_t size) {
    //如果需要移动的内存在 1M 以下,则不移动
    assertk((ptr_t) addr < HIGH_MEM + 4 * M);
    if (addr > (void *) (1 * M)) {
        void *new = (void *) block_alloc(size) + HIGH_MEM;
        assertk(new != NULL && (ptr_t) new < HIGH_MEM + 4 * M);
        q_memcpy(new, addr, size);
        return new;
    }
    return addr;
}

// 移动 memory map, shstrtab, strtab, symtab 结构到空闲内存块头
// 先移动地址最小的结构,防止覆盖其他结构
static void reload() {
    void *addr[3] = {bInfo.shstrtab.addr, bInfo.strtab.addr, bInfo.symtab.header};

    //排序
    for (int j = 0; j < 3; ++j) {
        for (int i = j + 1; i < 3; ++i) {
            if (addr[i] < addr[j]) {
                void *temp = addr[i];
                addr[i] = addr[j];
                addr[j] = temp;
            };
        }
    }

    for (int i = 0; i < 3; ++i) {
        if (addr[i] == bInfo.shstrtab.addr) {
            bInfo.shstrtab.addr = move(bInfo.shstrtab.addr, bInfo.shstrtab.size);
            assertk(bInfo.shstrtab.addr != 0);
        } else if (addr[i] == bInfo.strtab.addr) {
            bInfo.strtab.addr = move(bInfo.strtab.addr, bInfo.strtab.size);
            assertk(bInfo.strtab.addr != 0);
        } else {
            bInfo.symtab.header = move(bInfo.symtab.header, bInfo.symtab.size);
            assertk(bInfo.symtab.header != 0);
        }
    }
}

