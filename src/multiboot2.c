// TODO: 管理 1M 以下 内存
#include "multiboot2.h"
#include "lib/qlib.h"
#include "mm/mm.h"
#include "lib/qstring.h"
#include "elf.h"

struct bootInfo bInfo = {
        .shstrtab = {0},
        .strtab= {0},
        .symtab = {0}
};


//最原始的内存分配函数
//切割内核后的空闲内存块, 被切割的物理内存与虚拟内存直接映射
//内存不足返回 0
ptr_t split_mmap(uint32_t size) {
    size = MEM_ALIGN(size, 4);
    for_each_mmap {
        assertk(entry->zero == 0);
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE && entry->len >= size) {
            uint32_t addr = entry->addr;
            entry->addr += size;
            entry->len -= size;
            if (addr == bInfo.vmm_start) {
                bInfo.vmm_start = entry->addr;
                bInfo.mem_total -= size;
            };
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
    void *addr[4] = {bInfo.mmap, bInfo.shstrtab.addr, bInfo.strtab.addr, bInfo.symtab.header};

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
        if (addr[i] == bInfo.mmap) {
            bInfo.mmap = move(bInfo.mmap, bInfo.mmap->size);
        } else if (addr[i] == bInfo.shstrtab.addr) {
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
            bInfo.mem_total += entry->len;
        }
    }

}

static void parse_elf_section() {
    boot_tag_elf_sections_t *elf_symbols = bInfo.elf_symbols;
    elf32_sh_t *sh = (elf32_sh_t *) elf_symbols->sections;
    // 第一个 section header 为空,所有字段都是 0
    assertk(sh->sh_type == SHT_NULL);

    elf32_sh_t *string_table = &sh[elf_symbols->shndx];
    bInfo.shstrtab.addr = (void *) (string_table->sh_addr);
    bInfo.shstrtab.size = string_table->sh_size;
    assertk(bInfo.shstrtab.addr[0] == '\0');

    for (uint32_t num = elf_symbols->num; num > 0; num--) {
        switch (sh->sh_type) {
            case SHT_STRTAB:
                if (sh->sh_addr != (ptr_t) bInfo.shstrtab.addr) {
                    bInfo.strtab.addr = (void *) (sh->sh_addr);
                    bInfo.strtab.size = sh->sh_size;
                }
                break;
            case SHT_SYMTAB:
                //第一个项为空(所有字段为0)
                bInfo.symtab.header = (elf32_symbol_t *) (sh->sh_addr);
                bInfo.symtab.size = sh->sh_size;
                bInfo.symtab.entry_size = sizeof(elf32_symbol_t);
                break;
        }
        sh++;
    }
    assertk(bInfo.symtab.header->st_size == 0);
    assertk(bInfo.strtab.addr[0] == '\0');
}

void multiboot_init(multiboot_info_t *bia) {
    boot_tag_t *tag;
    //指向最大地址
    ptr_t tail = (ptr_t) bia + bia->total;
    // 第一个标签首地址
    tag = (boot_tag_t *) (bia + 1);

    while ((ptr_t) tag < tail) {
        switch (tag->type) {
            case MULTIBOOT_TAG_TYPE_MMAP:
                bInfo.mmap = (boot_tag_mmap_t *) tag;
                break;
            case MULTIBOOT_TAG_TYPE_APM:
                bInfo.apm = (boot_tag_apm_t *) tag;
                break;
            case MULTIBOOT_TAG_TYPE_ELF_SECTIONS:
                bInfo.elf_symbols = (boot_tag_elf_sections_t *) tag;
                break;
        }
        //Boot information 的 tags以 8 字节对齐
        // multiboot_tag 大小为 8 字节
        tag = tag + DIV_CEIL(tag->size, 8);
    }

    assertk(bInfo.mmap->entry_version == 0);
    assertk(bInfo.apm->type == MULTIBOOT_TAG_TYPE_APM);
    bInfo.vmm_start = MEM_ALIGN(K_END, 4);
    parse_elf_section();
    parse_mmap();
    reload();
}
