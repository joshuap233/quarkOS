#include <stdarg.h> // 可变参数
#include "lib/qstring.h"
#include "lib/qlib.h"
#include "types.h"
#include "drivers/vga.h"
#include "elf.h"
#include "multiboot2.h"

static void print_d(int64_t num) {
    uint8_t i = 0;
    char str[sizeof(uint64_t) + 1 + 1];//1 位存符号
    if (num < 0) {
        str[i++] = '-';
        num &= (BIT_MASK(uint64_t, 1) << 63);
    }
    q_utoa(num, str + i);
    vga_put_string(str);
}

static void print_u(uint64_t num) {
    char str[sizeof(uint64_t) + 1];
    q_utoa(num, str);
    vga_put_string(str);
}

static void print_pointer(void *p) {
    char str[sizeof(uint64_t) + 1 + 2] = "0x";
    hex((pointer_t) p, str + 2);
    vga_put_string(str);
}

static void print_hex(uint64_t x) {
    char str[sizeof(uint64_t) + 1 + 2] = "0x";
    hex(x, str + 2);
    vga_put_string(str);
}


struct stack_frame {
    struct stack_frame *ebp;
    uint32_t eip;
};

void stack_trace() {
    // 要求栈底为 caller 的 ebp 与 eip
    struct stack_frame *stk;
    asm volatile("movl %%ebp,%0":"=r"(stk));
    while (stk->ebp) {
        char *name = cur_func_name((pointer_t) stk->eip);
        printfk("ip: [%x], func: [%s]\n", stk->eip, name == NULL ? "NULL" : name);
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
    return NULL;
}

// 内核异常,停止运行
void panic() {
    disable_interrupt();
    stack_trace();
    halt();
}

#ifdef __i386__

__attribute__ ((format (printf, 1, 2))) void printfk(char *__restrict str, ...) {
    size_t str_len = q_strlen(str);
    va_list ap;
    va_start(ap, str);
    for (size_t i = 0; i < str_len; i++) {
        if (str[i] == '%' && (i + 1) < str_len) {
            switch (str[++i]) {
                case 'd':
                    print_d(va_arg(ap, int32_t));
                    break;
                case 'u':
                    print_u(va_arg(ap, uint32_t));
                    break;
                case 's':
                    vga_put_string(va_arg(ap, char*));
                    break;
                case 'c':
                    vga_put_char((char) va_arg(ap, int));
                    break;
                case 'p':
                    print_pointer(va_arg(ap, void*));
                    break;
                case 'x':
                    print_hex(va_arg(ap, uint32_t));
                    break;
                case 'l':
                    switch (str[++i]) {
                        case 'd':
                            print_d(va_arg(ap, int64_t));
                            break;
                        case 'u':
                            print_u(va_arg(ap, uint64_t));
                            break;
                        case 'x':
                            print_hex(va_arg(ap, uint64_t));
                            break;
                        default:
                            i--;
                    }
                    break;
                default:
                    i--;
                    vga_put_char(str[i]);
            }
        } else {
            vga_put_char(str[i]);
        }
    }
    va_end(ap);
}


#endif

