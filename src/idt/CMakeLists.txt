add_library(idt OBJECT idt.c isr.c idt.s)

set(CMAKE_C_FLAGS "-ffreestanding -O0 -Wall -Wextra -mgeneral-regs-only -g")

