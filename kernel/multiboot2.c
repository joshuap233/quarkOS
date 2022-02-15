#include <multiboot2.h>
#include <lib/qlib.h>
#include <elf.h>
#include <highmem.h>

struct bootInfo bInfo = {
        .shstrtab = {0},
        .strtab= {0},
        .symtab = {0},
        .rsdp = NULL
};


static void parse_elf_section() {
    boot_tag_elf_sections_t *elf_symbols = bInfo.elf_symbols;
    elf32_sh_t *sh = (elf32_sh_t *) elf_symbols->sections;
    // 第一个 section header 为空,所有字段都是 0
    assertk(sh->sh_type == SHT_NULL);
    elf32_sh_t *string_table = &sh[elf_symbols->shndx];
    bInfo.shstrtab.addr = (void *) (string_table->sh_addr);
    bInfo.shstrtab.size = string_table->sh_size;
    assertk(bInfo.shstrtab.addr[0] == '\0');

    assertk((ptr_t) (sh + elf_symbols->num * sizeof(elf32_sh_t)) < HIGH_MEM + 4 * M);
    for (uint32_t num = elf_symbols->num; num > 0; num--) {
        switch (sh->sh_type) {
            case SHT_STRTAB:
                if (sh->sh_addr != (ptr_t) bInfo.shstrtab.addr) {
                    assertk(sh->sh_addr + sh->sh_size < 4 * M);
                    bInfo.strtab.addr = (void *) (sh->sh_addr) + HIGH_MEM;
                    bInfo.strtab.size = sh->sh_size;
                }
                break;
            case SHT_SYMTAB: {
                //第一个项为空(所有字段为0)
                assertk(sh->sh_addr + sh->sh_size < 4 * M);
                bInfo.symtab.header = (elf32_symbol_t *) (sh->sh_addr) + HIGH_MEM;
                bInfo.symtab.size = sh->sh_size;
                bInfo.symtab.entry_size = sizeof(elf32_symbol_t);
                break;
            }
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
        assertk((ptr_t) tag < HIGH_MEM + 4 * M);

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
            case MULTIBOOT_TAG_TYPE_ACPI_OLD:
                bInfo.rsdp = &((boot_tag_acpi1 *) tag)->rsdp;
                break;
        }
        //Boot information 的 tags以 8 字节对齐
        // multiboot_tag 大小为 8 字节
        tag = tag + DIV_CEIL(tag->size, 8);
    }

    assertk(bInfo.mmap->entry_version == 0);
    assertk(bInfo.apm->type == MULTIBOOT_TAG_TYPE_APM);
    parse_elf_section();
}
