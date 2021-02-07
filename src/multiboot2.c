#include "multiboot2.h"
#include "qlib.h"
#include "qmath.h"
#include "mm.h"

multiboot_tag_mmap_t *g_mmap;
multiboot_tag_apm_t *g_apm;
uint32_t g_mem_total;



//计算所有可用物理内存大小
//移除内核以下可用内存
//使可用内存开始地址页对齐(不对齐部分直接抛弃)
void parse_memory_map() {
    pointer_t tail = (pointer_t) g_mmap + g_mmap->size;
    for (multiboot_mmap_entry_t *entry = g_mmap->entries; (pointer_t) entry < tail; entry++)
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
            if (entry->addr < K_END) {
                uint64_t end_addr = entry->addr + entry->len;
                if (end_addr < K_END) {
                    entry->type = MULTIBOOT_MEMORY_RESERVED;
                    continue;
                }
                entry->addr = K_END;
                entry->len = end_addr - entry->addr;
            }
            //使页地址向后对齐,无法对齐内存直接舍弃
            entry->addr = SIZE_ALIGN(entry->addr);
            g_mem_total += entry->len;
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
    parse_memory_map();
}
