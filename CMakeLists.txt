# 感谢: https://github.com/balsigergil/OSDev-Clion
cmake_minimum_required(VERSION 3.15)
project(quarkOS)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR i686)

ENABLE_LANGUAGE(ASM)

set(CMAKE_C_STANDARD 11)

add_subdirectory(kernel)
add_subdirectory(userspace)

# 下面的部分用于 Clion ide 代码检查
set(TOOLS $ENV{HOME}/OS/tool)
set(CMAKE_C_COMPILER ${TOOLS}/bin/i686-elf-gcc)
set(CMAKE_ASM_COMPILER ${TOOLS}/bin/i686-elf-gcc)
set(CMAKE_AR ${TOOLS}/bin/i686-elf-ar)
set(CMAKE_OBJCOPY ${TOOLS}/i686-elf-objcopy)
set(CMAKE_OBJDUMP ${TOOLS}/i686-elf-objdump)

