//
// Created by pjs on 2021/5/20.
//
#include <elf.h>
#include <fs/vfs.h>
#include <loader.h>
#include <mm/kmalloc.h>
#include <mm/page_alloc.h>
#include <task/task.h>

bool elf_check_header(elf32_header_t *elf_head) {
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

INLINE u32_t get_flags(struct elf32_sh *sh) {
    return sh->sh_flags == SHF_WRITE ? VM_UW : VM_UR;
}

void load_elf_exec(const char *path) {
    struct task_struct *task = CUR_TCB;
    assertk(task->mm);

    struct elf32_header *hdr;
    struct page *page, *page1, *page2;
    struct elf32_sh *sh, *str_sh;
    char *string, *name;
    size_t size;

    fd_t file = vfs_ops.open(path);
    assertk(file >= 0);

    page = vfs_ops.read_page(file, 0);
    hdr = page->data;
    if (elf_check_header(hdr)) {
        // 暂时只处理可执行文件
        assertk(hdr->e_type == ET_EXEC);
        assertk(hdr->e_shstrndx != SHN_UNDEF);
        // 取消用户空间虚拟内存映射
        vm_struct_unmaps(task->mm, true);

        page1 = vfs_ops.read_page(file, hdr->e_shoff);

        sh = page1->data + hdr->e_shoff % PAGE_SIZE;
        str_sh = &sh[hdr->e_shstrndx];
        assertk(str_sh->sh_type == SHT_STRTAB);

        page2 = vfs_ops.read_page(file, str_sh->sh_offset);
        string = page2->data + str_sh->sh_offset % PAGE_SIZE;

        assertk(string[0] == '\0');
        for (int i = 0; i < hdr->e_shnum; ++i) {
            if (sh->sh_type != SHT_NULL) {
                name = &string[sh->sh_name];
                struct page *tmp = vfs_ops.read_page(file, sh->sh_offset);
                vm_map_page(
                        task->mm->pgdir,
                        sh->sh_offset,
                        page_addr(tmp),
                        get_flags(sh)
                );
            }
            sh++;
        }
    }
    vfs_ops.close(file);
}
