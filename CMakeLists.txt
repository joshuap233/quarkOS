# 感谢: https://github.com/balsigergil/OSDev-Clion
cmake_minimum_required(VERSION 3.15)
project(quarkOS)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR i686)

ENABLE_LANGUAGE(ASM)

set(TOOLS $ENV{HOME}/OS/tool)
set(CMAKE_C_COMPILER ${TOOLS}/bin/i686-elf-gcc)
set(CMAKE_CXX_COMPILER ${TOOLS}/bin/i686-elf-g++)
set(CMAKE_ASM_COMPILER ${TOOLS}/bin/i686-elf-as)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "-ffreestanding -O0 -Wall -Wextra")

set(KERNEL_BIN ${CMAKE_PROJECT_NAME}.bin)
set(KERNEL_ISO ${CMAKE_PROJECT_NAME}.iso)

include_directories("${PROJECT_SOURCE_DIR}/src/include")
#AUX_SOURCE_DIRECTORY("${PROJECT_SOURCE_DIR}/src/" C_SOURCE)

set(S_SOURCE src/boot.s src/gdt.s)
set(C_SOURCE src/kernel.c src/gdt.c src/idt.c src/multiboot2.c)

add_executable(${KERNEL_BIN} ${S_SOURCE} ${C_SOURCE} ${mylib} ${drivers})
set_target_properties(${KERNEL_BIN} PROPERTIES LINK_FLAGS "-T ${CMAKE_SOURCE_DIR}/linker.ld -ffreestanding -O0 -nostdlib")
target_link_libraries(${KERNEL_BIN} PRIVATE gcc)

# 添加库文件
add_subdirectory(${PROJECT_SOURCE_DIR}/src/lib)
target_link_libraries(${KERNEL_BIN} PUBLIC mylib)
target_include_directories(
         ${KERNEL_BIN} PUBLIC
        "${PROJECT_SOURCE_DIR}/src/lib"
)

add_subdirectory(${PROJECT_SOURCE_DIR}/src/drivers)
target_link_libraries(${KERNEL_BIN} PUBLIC drivers)
target_include_directories(
        ${KERNEL_BIN} PUBLIC
        "${PROJECT_SOURCE_DIR}/src/drivers"
)

## 生成 iso
add_custom_target(
        ${KERNEL_ISO}
        DEPENDS ${KERNEL_BIN}
        COMMAND ../generate-iso.sh ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL_BIN}
        COMMENT "正在构建 iso 文件..."
        BYPRODUCTS ${KERNEL_ISO}
)

#add_dependencies(${KERNEL_ISO} ${KERNEL_BIN})

add_custom_target(run ALL DEPENDS ${KERNEL_BIN} ${KERNEL_ISO})