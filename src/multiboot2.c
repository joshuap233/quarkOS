#include "multiboot2.h"
#include "qlib.h"
#include "qmath.h"
#include "mm.h"
#include "qstring.h"

multiboot_tag_mmap_t *g_mmap;
multiboot_tag_apm_t *g_apm;
uint32_t g_mem_total; // 可用物理内存大小
pointer_t g_mmap_tail; //memory map 块末尾
pointer_t g_vmm_start;//vmm 虚拟内存开始地址, vmm_start 以下的虚拟内存需要预留且与物理内存直接映射

//切割内核后的空闲内存块, 被切割的物理内存与虚拟内存直接映射
//内存不足返回 0
pointer_t split_mmap(uint32_t size) {
    for (multiboot_mmap_entry_t *entry = g_mmap->entries; (pointer_t) entry < g_mmap_tail; entry++) {
        assertk(entry->zero == 0);
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE && entry->len >= size) {
            uint32_t addr = entry->addr;
            entry->addr += size;
            entry->len -= size;
            if (addr == g_vmm_start) g_vmm_start = entry->addr;
            return addr;
        }
    }
    return 0;
}

//移动 memory map 结构移动到可用内存块开头
void reload_mmap() {
    multiboot_tag_mmap_t *new_g_map = (multiboot_tag_mmap_t *) split_mmap(g_mmap->size);
    assertk((pointer_t) new_g_map != 0);
    q_memcpy(new_g_map, g_mmap, g_mmap->size);
    g_mmap = new_g_map;
    g_mmap_tail = (pointer_t) g_mmap + g_mmap->size - 1;
}

//计算所有可用物理内存大小(不包括内核以下部分)
//移除内核以下可用内存
void parse_memory_map() {
    for (multiboot_mmap_entry_t *entry = g_mmap->entries; (pointer_t) entry < g_mmap_tail; entry++) {
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
            g_mem_total += entry->len;
        }
    }

}


void multiboot_init(multiboot_info_t *bia) {
    multiboot_tag_t *tag;
    //指向最大地址
    pointer_t tail = (pointer_t) bia + bia->total;
    // 第一个标签首地址
    tag = (multiboot_tag_t *) (bia + 1);

    while ((pointer_t) tag < tail) {
        switch (tag->type) {
            case MULTIBOOT_TAG_TYPE_MMAP:
                g_mmap = (multiboot_tag_mmap_t *) tag;
                break;
            case MULTIBOOT_TAG_TYPE_APM:
                g_apm = (multiboot_tag_apm_t *) tag;
                break;
        }
        //Boot information 的 tags以 8 字节对齐
        // multiboot_tag 大小为 8 字节
        tag = tag + divUc(tag->size, 8);
    }

    assertk(g_mmap->entry_version == 0);
    assertk(g_apm->type == MULTIBOOT_TAG_TYPE_APM);
    g_mmap_tail = (pointer_t) g_mmap + g_mmap->size - 1;
    g_vmm_start = K_END;
    parse_memory_map();
    reload_mmap();
}
