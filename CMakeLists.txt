# 感谢: https://github.com/balsigergil/OSDev-Clion
cmake_minimum_required(VERSION 3.15)
project(quarkOS)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR i686)

ENABLE_LANGUAGE(ASM)

set(TOOLS $ENV{HOME}/OS/tool)
set(CMAKE_C_COMPILER ${TOOLS}/bin/i686-elf-gcc)
set(CMAKE_ASM_COMPILER ${TOOLS}/bin/i686-elf-as)
set(OBJCOPY ${TOOLS}/i686-elf-objcopy)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "-ffreestanding -O0 -Wall -Wextra -g -mgeneral-regs-only")
set(CMAKE_ASM_FLAGS "-g")

set(KERNEL_BIN ${CMAKE_PROJECT_NAME}.bin)
set(KERNEL_ISO ${CMAKE_PROJECT_NAME}.iso)
set(INCLUDE ${PROJECT_SOURCE_DIR}/src/include)
include_directories(${INCLUDE})

set(S_SOURCE src/boot.s)
set(C_SOURCE src/kernel.c src/multiboot2.c src/bio.c)

add_executable(${KERNEL_BIN} ${C_SOURCE} ${S_SOURCE} ${mylib} ${drivers})
set_target_properties(${KERNEL_BIN} PROPERTIES LINK_FLAGS "-T ${CMAKE_SOURCE_DIR}/linker.ld -ffreestanding -O0 -nostdlib -g")
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


add_subdirectory(${PROJECT_SOURCE_DIR}/src/mm)
target_link_libraries(${KERNEL_BIN} PUBLIC mm)
target_include_directories(
        ${KERNEL_BIN} PUBLIC
        "${PROJECT_SOURCE_DIR}/src/mm"
)

add_subdirectory(${PROJECT_SOURCE_DIR}/src/sched)
target_link_libraries(${KERNEL_BIN} PUBLIC sched)
target_include_directories(
        ${KERNEL_BIN} PUBLIC
        "${PROJECT_SOURCE_DIR}/src/sched"
)

add_subdirectory(${PROJECT_SOURCE_DIR}/src/x86)
target_link_libraries(${KERNEL_BIN} PUBLIC x86)
target_include_directories(
        ${KERNEL_BIN} PUBLIC
        "${PROJECT_SOURCE_DIR}/src/x86"
)

add_subdirectory(${PROJECT_SOURCE_DIR}/src/fs)
target_link_libraries(${KERNEL_BIN} PUBLIC fs)
target_include_directories(
        ${KERNEL_BIN} PUBLIC
        "${PROJECT_SOURCE_DIR}/src/fs"
)


## 生成 iso
add_custom_target(
        ${KERNEL_ISO}
        DEPENDS ${KERNEL_BIN}
        COMMAND ../generate-iso.sh ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL_BIN}
        COMMENT "正在构建 iso 文件..."
        BYPRODUCTS ${KERNEL_ISO}
)

add_custom_target(run ALL DEPENDS ${KERNEL_BIN} ${KERNEL_ISO})
