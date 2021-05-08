//
// Created by pjs on 2021/5/5.
//
// 最原始的物理内存管理器
// TODO: 管理 1M 以下 内存?
#include "mm/block_alloc.h"
#include "multiboot2.h"
#include "lib/qlib.h"
#include "mm/mm.h"
#include "lib/qstring.h"


typedef struct blockInfo {
    list_head_t head;
    u32_t size; // 块大小,包括该块头
} blockInfo_t;

static struct block_allocator {
    list_head_t head;
    ptr_t addr;  // 管理器起始地址, <= addr 部分内存需要直接映射
    u32_t total;
} allocator;

#define entry(ptr) list_entry(ptr, blockInfo_t, head)

static void reload();

void memBlock_init() {
    allocator.total = 0;
    list_header_init(&allocator.head);

    boot_mmap_entry_t *entry;
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
            if (entry->len >= sizeof(blockInfo_t)) {
                blockInfo_t *info = (void *) MEM_ALIGN(entry->addr, 4);
                info->size = entry->len - ((ptr_t) info - entry->addr);
                list_add_prev(&info->head, &allocator.head);
            }
            allocator.total += entry->len;
        }
    }

    // blockInfo_t 按地址大小排序
    list_head_t *hdr, *next;
    list_for_each_del(hdr, next, &allocator.head) {
        list_head_t *tmp = hdr->prev;
        for (; tmp != &allocator.head && (ptr_t) entry(hdr) < (ptr_t) entry(tmp); tmp = tmp->prev);
        if (tmp != hdr->prev) {
            list_del(hdr);
            list_add_next(hdr, tmp);
        }
    }
    allocator.addr = (ptr_t) allocator.head.next;
    reload();
}


ptr_t block_alloc(u32_t size) {
    assertk(allocator.total >= size);
    size = MEM_ALIGN(size, 4);
    list_head_t *hdr;
    list_for_each(hdr, &allocator.head) {
        blockInfo_t *info = entry(hdr);
        u32_t blkSize = info->size;
        if (blkSize >= size) {
            list_head_t *prev = info->head.prev;
            list_del(&info->head);
            if (blkSize - size > sizeof(blockInfo_t)) {
                blockInfo_t *new = (void *) info + size;
                new->size = info->size - size;
                list_add_next(&new->head, prev);
                allocator.total -= size;
            } else {
                allocator.total -= info->size;
            }
            allocator.addr = (ptr_t) allocator.head.next;
            return (ptr_t) info;
        }
    }
    assertk(0);
    return 0;
}

ptr_t block_alloc_align(u32_t size, u32_t align) {
    assertk(align > 4 && (align & 0x3) == 0);
    ptr_t addr = block_alloc(size + align - 1);
    return PAGE_ALIGN(addr);
}

u32_t block_size() {
    return allocator.total;
}

u32_t block_start() {
    return allocator.addr;
}

//移动 信息块
static void *move(void *addr, size_t size) {
    //如果需要移动的内存在 1M 以下,则不移动
    if (addr > (void *) (1 * M)) {
        void *new = (void *) block_alloc(size);
        assertk(new != 0);
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
