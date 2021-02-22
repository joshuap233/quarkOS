// TODO: 管理 1M 以下 内存
#include "multiboot2.h"
#include "klib/qlib.h"
#include "klib/qmath.h"
#include "mm/mm.h"
#include "klib/qstring.h"
#include "elf.h"
#include "stack_trace.h"

multiboot_tag_mmap_t *g_mmap;
multiboot_tag_apm_t *g_apm;
multiboot_tag_elf_sections_t *elf_symbols;
elf_string_table_t g_shstrtab = {0}, g_strtab = {0};
elf_symbol_table_t g_symtab = {0};

uint32_t g_mem_total; // 可用物理内存大小
pointer_t g_mmap_tail; //memory map 块末尾
pointer_t g_vmm_start;//vmm 虚拟内存开始地址, vmm_start 以下的虚拟内存需要预留且与物理内存直接映射

//最原始的内存分配函数
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
static void reload_mmap() {
    //如果 g_mmap 在 1M 内存以下,则不移动
    if (g_mmap > (multiboot_tag_mmap_t *) (1 * M)) {
        multiboot_tag_mmap_t *new_g_map = (multiboot_tag_mmap_t *) split_mmap(g_mmap->size);
        assertk((pointer_t) new_g_map != 0);
        q_memcpy(new_g_map, g_mmap, g_mmap->size);
        g_mmap = new_g_map;
        g_mmap_tail = (pointer_t) g_mmap + g_mmap->size - 1;
    }
}

//计算所有可用物理内存大小(不包括内核以下部分)
//移除内核以下可用内存
static void parse_mmap() {
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

static void parse_elf_section() {
    elf32_sh_t *sh = (elf32_sh_t *) elf_symbols->sections;
    // 第一个 section header 为空,所有字段都是 0
    assertk(sh->sh_type == SHT_NULL);

    elf32_sh_t *string_table = &sh[elf_symbols->shndx];
    g_shstrtab.addr = (void *) (string_table->sh_addr);
    g_shstrtab.size = string_table->sh_size;
    assertk(g_shstrtab.addr[0] == '\0');

    for (uint32_t num = elf_symbols->num; num > 0; num--) {
        switch (sh->sh_type) {
            case SHT_STRTAB:
                if (sh->sh_addr != (pointer_t) g_shstrtab.addr) {
                    g_strtab.addr = (void *) (sh->sh_addr);
                    g_strtab.size = sh->sh_size;
                }
                break;
            case SHT_SYMTAB:
                //第一个项为空(所有字段为0)
                g_symtab.header = (elf32_symbol_t *) (sh->sh_addr);
                g_symtab.size = sh->sh_size;
                g_symtab.entry_size = sizeof(elf32_symbol_t);
                break;
        }
        sh++;
    }
    assertk(g_symtab.header->st_size == 0);
    assertk(g_strtab.addr[0] == '\0');
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
            case MULTIBOOT_TAG_TYPE_ELF_SECTIONS:
                elf_symbols = (multiboot_tag_elf_sections_t *) tag;
                break;
        }
        //Boot information 的 tags以 8 字节对齐
        // multiboot_tag 大小为 8 字节
        tag = tag + DIV_CEIL(tag->size, 8);
    }

    assertk(g_mmap->entry_version == 0);
    assertk(g_apm->type == MULTIBOOT_TAG_TYPE_APM);
    g_mmap_tail = (pointer_t) g_mmap + g_mmap->size - 1;
    g_vmm_start = K_END;
    parse_elf_section();
    parse_mmap();
    //最后调用 reload_mmap, 防止 mmap 覆盖其他 multiboot 信息
    reload_mmap();
}
