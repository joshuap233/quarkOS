// TODO: 管理 1M 以下 内存
#include "multiboot2.h"
#include "lib/qlib.h"
#include "mm/mm.h"
#include "lib/qstring.h"
#include "elf.h"


multiboot_tag_elf_sections_t *elf_symbols;

multiboot_tag_mmap_t *g_mmap;
multiboot_tag_apm_t *g_apm;
elf_string_table_t g_shstrtab = {0}, g_strtab = {0};
elf_symbol_table_t g_symtab = {0};

uint32_t g_mem_total;  // 可用物理内存大小
pointer_t g_vmm_start; //vmm 虚拟内存开始地址, vmm_start 以下的虚拟内存需要预留且与物理内存直接映射


//最原始的内存分配函数
//切割内核后的空闲内存块, 被切割的物理内存与虚拟内存直接映射
//内存不足返回 0
pointer_t split_mmap(uint32_t size) {
    for_each_mmap {
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


//移动 信息块
static void *move(void *addr, size_t size) {
    //如果需要移动的内存在 1M 以下,则不移动
    if (addr > (void *) (1 * M)) {
        void *new = (void *) split_mmap(size);
        assertk(new != 0);
        q_memcpy(new, addr, size);
        return new;
    }
    return addr;
}


//移动 memory map, shstrtab, strtab, symtab 结构到空闲内存块头
//先移动地址最小的结构,防止覆盖其他结构
static void reload() {
    void *addr[4] = {g_mmap, g_shstrtab.addr, g_strtab.addr, g_symtab.header};

    //排序
    for (int j = 0; j < 3; ++j) {
        for (int i = j + 1; i < 4; ++i) {
            if (addr[i] < addr[j]) {
                void *temp = addr[i];
                addr[i] = addr[j];
                addr[j] = temp;
            };
        }
    }

    for (int i = 0; i < 4; ++i) {
        if (addr[i] == g_mmap) {
            g_mmap = move(g_mmap, g_mmap->size);
        } else if (addr[i] == g_shstrtab.addr) {
            g_shstrtab.addr = move(g_shstrtab.addr, g_shstrtab.size);
            assertk(g_shstrtab.addr != 0);
        } else if (addr[i] == g_strtab.addr) {
            g_strtab.addr = move(g_strtab.addr, g_strtab.size);
            assertk(g_strtab.addr != 0);
        } else {
            g_symtab.header = move(g_symtab.header, g_symtab.size);
            assertk(g_symtab.header != 0);
        }
    }

}

//计算所有可用物理内存大小(不包括内核以下部分)
//移除内核以下可用内存
static void parse_mmap() {
    for_each_mmap {
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
    g_vmm_start = K_END;
    parse_elf_section();
    parse_mmap();
    reload();
}
