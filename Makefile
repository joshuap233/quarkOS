# 参考 https://wiki.osdev.org/Why_do_I_need_a_Cross_Compiler%3F
# 不应该使用的参数
# -m32, -m64 (compiler) -melf_i386, -melf_x86_64 (linker) -32, -64 (assembler)
# -nostdinc -fno-builtin -fno-stack-protector

.PHONY: clean run

CC := i686-elf-gcc
AS := i686-elf-as
DIR_INC   := ./src/include
VPATH     := ./build
C_FLAGS   := -std=gnu11 -ffreestanding -O2 -Wall -Wextra -I $(DIR_INC)
LD_SCRIPT := linker.ld
LD_FLAGS  := -T $(LD_SCRIPT) -ffreestanding -O2 -nostdlib -lgcc

C_SOURCES := $(shell find ./src -type f -name "*.c")
S_SOURCES := $(shell find ./src -type f -name "*.s")
# $(C_SOURCES) 内的 *.s 替换为 *.o
C_OBJECTS := $(addprefix build/,$(notdir $(patsubst %.c,%.o,$(C_SOURCES))))
S_OBJECTS := $(addprefix build/,$(notdir $(patsubst %.s,%.o,$(S_SOURCES))))

C_SOURCES_DIR := $(dir $(patsubst %.c,%.o,$(C_SOURCES)))
S_SOURCES_DIR := $(dir $(patsubst %.s,%.o,$(S_SOURCES)))

VPATH = $(C_SOURCES_DIR) $(S_SOURCES_DIR)


all: quarkOS.iso

build/quarkOS.bin: $(C_OBJECTS) $(S_OBJECTS)
	$(CC) -o build/quarkOS.bin $(S_OBJECTS) $(C_OBJECTS) $(LD_FLAGS) $(C_FLAGS)

# makefile 静态模式
# https://seisman.github.io/how-to-write-makefile/rules.html
$(C_OBJECTS): build/%.o: %.c
	# $(filter %$<,$(C_SOURCES)) 根据源文件名(不包括目录),查找该文件所在目录
	$(CC) $(C_FLAGS) -c $(filter %$<,$(C_SOURCES)) -o $@

$(S_OBJECTS): build/%.o: %.s
	$(AS) $(filter %$<,$(S_SOURCES))  -o $@

quarkOS.iso: build/quarkOS.bin ./isodir/boot/grub/grub.cfg
	./multibootCheck.sh
	cp build/quarkOS.bin isodir/boot
	grub-mkrescue -o build/quarkOS.iso isodir

run:
	qemu-system-i386 -cdrom build/quarkOS.iso

clean:
	-rm build/*

