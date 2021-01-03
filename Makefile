FLAGS = -ffreestanding
# 参考 https://wiki.osdev.org/Why_do_I_need_a_Cross_Compiler%3F
LD_FLAGS = -nostdlib -nostartfiles -nodefaultlibs -lgcc
# 连接的 flag
# 不应该使用的参数
# -m32, -m64 (compiler) -melf_i386, -melf_x86_64 (linker) -32, -64 (assembler)
# -nostdinc -fno-builtin -fno-stack-protector

.PHONY: multibootCheck


myos.bin: boot.o kernel.o
	i686-elf-gcc -T linker.ld -o myos.bin -ffreestanding -O2 -nostdlib boot.o kernel.o -lgcc
	./multibootCheck.sh

boot.o: boot.s
	i686-elf-as boot.s -o boot.o

kernel.o: kernel.c
	i686-elf-gcc -c kernel.c -o kernel.o -std=gnu11 -ffreestanding -O2 -Wall -Wextra

clean:
	-rm boot.o kernel.o myos.bin

