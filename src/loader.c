//
// Created by pjs on 2021/5/20.
//
#include <elf.h>
#include <fs/vfs.h>

typedef elf32_header_t elf_header_t;


bool elf_check_header(elf_header_t *elf_head) {
    //TODO:错误处理
    if (elf_head->e_ident[EI_MAG0] != ELF_MAG0)
        return false;
    if (elf_head->e_ident[EI_MAG1] != ELF_MAG1)
        return false;
    if (elf_head->e_ident[EI_MAG2] != ELF_MAG2)
        return false;
    if (elf_head->e_ident[EI_MAG3] != ELF_MAG3)
        return false;

    if (elf_head->e_ident[EI_CLASS] != ELF_CLASS32)
        return false;
    if (elf_head->e_ident[EI_DATA] != ELF_DATA2LSB)
        return false;
    if (elf_head->e_ident[EI_VERSION] != EV_CURRENT)
        return false;
    if (elf_head->e_machine != EM_386)
        return false;
    if (elf_head->e_type != ET_REL && elf_head->e_type != ET_EXEC)
        return false;
    return true;
}


void do_elf_load(char *path) {

}
