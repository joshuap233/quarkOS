#include <lib/qlib.h>
#include <types.h>
#include <elf.h>
#include <multiboot2.h>
#include <terminal.h>
#include <x86.h>

struct stack_frame {
    struct stack_frame *ebp;
    uint32_t eip;
};

void stack_trace() {
    // 要求栈底为 caller 的 ebp 与 eip
    struct stack_frame *stk;
    asm volatile("movl %%ebp,%0":"=r"(stk));
    while (stk->ebp) {
        char *name = cur_func_name((ptr_t) stk->eip);
        printfk("ip: [%x], func: [%s]\n", stk->eip, name == NULL ? "NULL" : name);
        stk = stk->ebp;
    }
}

//addr 为函数中指令的地址
char *cur_func_name(ptr_t addr) {
    for (uint32_t i = 0; i < (bInfo.symtab.size / bInfo.symtab.entry_size); ++i) {
        if (ELF32_ST_TYPE(bInfo.symtab.header[i].st_info) == STT_FUNC) {
            elf32_symbol_t entry = bInfo.symtab.header[i];
            if ((addr >= entry.st_value) && (addr <= entry.st_value + entry.st_size)) {
                return (void *) &bInfo.strtab.addr[entry.st_name];
            }
        }
    }
    return NULL;
}

// 内核异常,停止运行
void panic() {
    disable_interrupt();
    stack_trace();
    halt();
}

u8_t log2(uint16_t val) {
    u8_t cnt = 0;
    while (val != 1) {
        cnt++;
        val >>= 1;
    }
    return cnt;
}
