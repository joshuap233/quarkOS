//
// Created by pjs on 2021/5/20.
//
#include <elf.h>
#include <fs/vfs.h>
#include <loader.h>
#include <mm/page_alloc.h>
#include <task/task.h>
#include <lib/qstring.h>

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
    assertk(elf_head->e_shentsize == sizeof(struct elf32_sh));
    assertk(elf_head->e_phentsize == sizeof(elf32_phdr_t));
    return true;
}

INLINE u32_t get_flags(struct elf32_phdr *pgh) {
    assertk(!(pgh->p_flags & PF_MASKPROC));
    return (pgh->p_flags & PF_W) ? VM_URW : VM_UR;
}

void load_elf_exec(const char *path) {
    struct mm_struct *mm = CUR_TCB->mm;
    assertk(mm);

    struct elf32_header *hdr;
    struct page *page, *page1;
    struct elf32_phdr *pgh;
    struct vm_area *area;
    pde_t *pgdir = mm->pgdir;
    u32_t flag;

    fd_t file = vfs_ops.open(path);
    assertk(file >= 0);

    page = vfs_ops.read_page(file, 0);
    hdr = page->data;
    if (elf_check_header(hdr)) {
        // 暂时只处理可执行文件
        assertk(hdr->e_type == ET_EXEC);
        assertk(hdr->e_shstrndx != SHN_UNDEF);
        // 取消用户空间虚拟内存映射
        vm_struct_unmaps(mm);
        bzero(pgdir, PAGE_SIZE / 4 * 3);
        bzero(mm, sizeof(mm_struct_t));
        mm->pgdir = pgdir;

        // 程序头表
        page1 = vfs_ops.read_page(file, hdr->e_phoff);
        pgh = page1->data + hdr->e_phoff % PAGE_SIZE;

        for (int i = 0; i < hdr->e_phnum; ++i) {
            if (pgh->p_type != PT_NULL) {
                assertk(pgh->p_type == PT_LOAD);

                ptr_t va = pgh->p_vaddr;
                size_t size = PAGE_ALIGN(pgh->p_memsz);
                ptr_t pa = alloc_page(size);
                flag = get_flags(pgh) | VM_PRES;
                vm_maps(va, pa, flag, size);

                if (pgh->p_filesz != pgh->p_memsz) {
                    assertk(mm->bss.size == 0);
                    assertk(pgh->p_filesz == 0);

                    area = &mm->bss;
                    bzero((void *) va, size);
                } else {
                    if (pgh->p_flags & PF_X) {
                        area = &mm->text;
                    } else if (pgh->p_flags & PF_W) {
                        area = &mm->data;
                    } else {
                        area = &mm->rodata;
                    }
                    assertk(area->size == 0);
                    vfs_ops.lseek(file, pgh->p_offset, SEEK_SET);
                    vfs_ops.read(file, (void *) va, pgh->p_memsz);
                }

                area->flag = flag;
                area->va = va;
                area->size = size;
                mm->size += area->size;
            }
            pgh++;
        }
    }

    vm_brk_init(mm);
    vm_stack_init(mm);

    mm->size += mm->brk.size + mm->stack.size;
    vfs_ops.close(file);

    sys_context_t *sysContext = CUR_TCB->sysContext;
    assertk(sysContext);

    ptr_t esp = mm->stack.va + STACK_SIZE;
    sysContext->esp = esp;
    sysContext->eip = hdr->e_entry;
}
