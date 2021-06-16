# 感谢: https://github.com/balsigergil/OSDev-Clion
cmake_minimum_required(VERSION 3.15)
project(quarkOS)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR i686)

ENABLE_LANGUAGE(ASM)

set(CMAKE_C_STANDARD 11)

add_subdirectory(kernel)
add_subdirectory(userspace)



