//
// Created by pjs on 2021/2/21.
//
#include "stack_trace.h"
#include "types.h"
#include "elf.h"
#include "multiboot2.h"
#include "klib/qlib.h"

struct stack_frame {
    struct stack_frame *ebp;
    uint32_t eip;
};

void stack_trace() {
    // 要求栈底为 caller 的 ebp 与 eip
    struct stack_frame *stk;
    asm ("movl %%ebp,%0":"=r"(stk));
    while (stk->ebp) {
        printfk("ip: [%x], func: [%s]\n", stk->eip, cur_func_name((pointer_t) stk->eip));
        stk = stk->ebp;
    }
}

//addr 为函数中指令的地址
char *cur_func_name(pointer_t addr) {
    for (uint32_t i = 0; i < (g_symtab.size / g_symtab.entry_size); ++i) {
        if (ELF32_ST_TYPE(g_symtab.header[i].st_info) == STT_FUNC) {
            elf32_symbol_t entry = g_symtab.header[i];
            if ((addr >= entry.st_value) && (addr <= entry.st_value + entry.st_size)) {
                return &g_strtab.addr[entry.st_name];
            }
        }
    }
}