# 感谢: https://github.com/balsigergil/OSDev-Clion
cmake_minimum_required(VERSION 3.15)
project(kernel)


set(TOOLS $ENV{HOME}/OS/tool)
set(CMAKE_C_COMPILER ${TOOLS}/bin/i686-elf-gcc)
set(CMAKE_ASM_COMPILER ${TOOLS}/bin/i686-elf-gcc)
set(CMAKE_AR ${TOOLS}/bin/i686-elf-ar)
set(CMAKE_OBJCOPY ${TOOLS}/i686-elf-objcopy)
set(CMAKE_OBJDUMP ${TOOLS}/i686-elf-objdump)

set(CMAKE_C_FLAGS "-ffreestanding -O0 -Wall -Wextra -g -mgeneral-regs-only")
set(CMAKE_ASM_FLAGS "-g")

set(KERNEL_BIN ${CMAKE_PROJECT_NAME}.bin)
set(KERNEL_ISO ${CMAKE_PROJECT_NAME}.iso)
set(INCLUDE ${PROJECT_SOURCE_DIR}/include)
include_directories(${INCLUDE})

set(S_SOURCE boot.s ap_boot.s ap_boot.s)
set(C_SOURCE kernel.c multiboot2.c loader.c terminal.c)

add_executable(${KERNEL_BIN} ${C_SOURCE} ${S_SOURCE} ${mylib} ${drivers})
set_target_properties(${KERNEL_BIN} PROPERTIES LINK_FLAGS "-T ${PROJECT_SOURCE_DIR}/linker.ld -ffreestanding -O0 -nostdlib -g")
target_link_libraries(${KERNEL_BIN} PRIVATE gcc)

# 添加库文件
add_subdirectory(${PROJECT_SOURCE_DIR}/lib)
target_link_libraries(${KERNEL_BIN} PUBLIC mylib)
target_include_directories(
         ${KERNEL_BIN} PUBLIC
        "${PROJECT_SOURCE_DIR}/lib"
)

add_subdirectory(${PROJECT_SOURCE_DIR}/drivers)
target_link_libraries(${KERNEL_BIN} PUBLIC drivers)
target_include_directories(
        ${KERNEL_BIN} PUBLIC
        "${PROJECT_SOURCE_DIR}/drivers"
)


add_subdirectory(${PROJECT_SOURCE_DIR}/mm)
target_link_libraries(${KERNEL_BIN} PUBLIC mm)
target_include_directories(
        ${KERNEL_BIN} PUBLIC
        "${PROJECT_SOURCE_DIR}/mm"
)

add_subdirectory(${PROJECT_SOURCE_DIR}/task)
target_link_libraries(${KERNEL_BIN} PUBLIC sched)
target_include_directories(
        ${KERNEL_BIN} PUBLIC
        "${PROJECT_SOURCE_DIR}/task"
)

add_subdirectory(${PROJECT_SOURCE_DIR}/x86)
target_link_libraries(${KERNEL_BIN} PUBLIC x86)
target_include_directories(
        ${KERNEL_BIN} PUBLIC
        "${PROJECT_SOURCE_DIR}/x86"
)

add_subdirectory(${PROJECT_SOURCE_DIR}/fs)
target_link_libraries(${KERNEL_BIN} PUBLIC fs)
target_include_directories(
        ${KERNEL_BIN} PUBLIC
        "${PROJECT_SOURCE_DIR}/fs"
)


add_subdirectory(${PROJECT_SOURCE_DIR}/syscall)
target_link_libraries(${KERNEL_BIN} PUBLIC syscall)
target_include_directories(
        ${KERNEL_BIN} PUBLIC
        "${PROJECT_SOURCE_DIR}/syscall"
)

#add_subdirectory(${PROJECT_SOURCE_DIR}/net)
#target_link_libraries(${KERNEL_BIN} PUBLIC net)
#target_include_directories(
#        ${KERNEL_BIN} PUBLIC
#        "${PROJECT_SOURCE_DIR}/net"
#)


## 生成 iso
add_custom_command(
        OUTPUT ${KERNEL_ISO}
        COMMAND ${CMAKE_SOURCE_DIR}/generate-iso.sh ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL_BIN}
        COMMENT "正在构建 iso 文件..."
        DEPENDS ${KERNEL_BIN}
)

add_custom_target(kernel ALL DEPENDS ${KERNEL_BIN} ${KERNEL_ISO})

