#include "multiboot2.h"
#include "qlib.h"
#include "qmath.h"

multiboot_tag_mmap_t *mmap;
multiboot_tag_apm_t *apm;

void parse_memory_map(void (*alloc)(pointer_t, pointer_t)) {
    pointer_t tail = (pointer_t) mmap + mmap->size;
    for (multiboot_mmap_entry_t *entry = mmap->entries; (pointer_t) entry < tail; entry++) {
        assertk(entry->zero == 0);
        switch (entry->type) {
            case MULTIBOOT_MEMORY_AVAILABLE:
                alloc(entry->addr, entry->len);
                break;
        }
    }
}


void parse_boot_info(multiboot_info_t *bia) {
    multiboot_tag_t *tag;
    //指向最大地址
    pointer_t tail = (pointer_t) bia + bia->total;
    // 第一个标签首地址
    tag = (multiboot_tag_t *) (bia + 1);

    while ((pointer_t) tag < tail) {
        switch (tag->type) {
            case MULTIBOOT_TAG_TYPE_MMAP:
                mmap = (multiboot_tag_mmap_t *) tag;
                break;
            case MULTIBOOT_TAG_TYPE_APM:
                apm = (multiboot_tag_apm_t *) tag;
                break;
        }
        //Boot information 的 tags以 8 字节对齐
        // multiboot_tag 大小为 8 字节
        tag = tag + divUc(tag->size, 8);
    }

    assertk(mmap->entry_version == 0);
    assertk(apm->type == MULTIBOOT_TAG_TYPE_APM);
}
