add_library(idt OBJECT idt.c handle.c idt.s pic.c timer.c)

set(CMAKE_C_FLAGS "-ffreestanding -O0 -Wall -Wextra -mgeneral-regs-only -mno-red-zone")

