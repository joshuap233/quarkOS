add_library(fs OBJECT writeback.c vfs.c hash.c)

add_subdirectory(${PROJECT_SOURCE_DIR}/fs/ext2)
target_link_libraries(${KERNEL_BIN} PUBLIC ext2)
target_include_directories(
        ${KERNEL_BIN} PUBLIC
        "${PROJECT_SOURCE_DIR}/fs/ext2"
)



