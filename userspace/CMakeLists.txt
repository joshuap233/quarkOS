cmake_minimum_required(VERSION 3.15)
project(userspace)

set(TOOLS $ENV{HOME}/OS/tool)
set(CMAKE_C_COMPILER ${TOOLS}/bin/i686-elf-gcc)
set(CMAKE_ASM_COMPILER ${TOOLS}/bin/i686-elf-gcc)
set(CMAKE_AR ${TOOLS}/bin/i686-elf-ar)
set(CMAKE_OBJCOPY ${TOOLS}/i686-elf-objcopy)
set(CMAKE_OBJDUMP ${TOOLS}/i686-elf-objdump)

set(CMAKE_C_FLAGS "-ffreestanding -O0 -Wall -Wextra -g")
set(CMAKE_ASM_FLAGS "-g")

set(INCLUDE ${CMAKE_SOURCE_DIR}/kernel/include)

include_directories(${INCLUDE})

set(SH_BIN sh.bin)

add_executable(${SH_BIN} lib.S lib.c sh.c)
set_target_properties(${SH_BIN} PROPERTIES LINK_FLAGS "-T ${PROJECT_SOURCE_DIR}/linker.ld -ffreestanding -O0 -nostdlib -g")
target_link_libraries(${SH_BIN} PRIVATE gcc)
