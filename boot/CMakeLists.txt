# Boot组件构建配置

# MBR目标
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/mbr.bin
    COMMAND nasm -I ${CMAKE_CURRENT_SOURCE_DIR}/include 
                 ${CMAKE_CURRENT_SOURCE_DIR}/mbr.asm 
                 -o ${CMAKE_CURRENT_BINARY_DIR}/mbr.bin
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mbr.asm
    COMMENT "Building MBR"
    VERBATIM
)

add_custom_target(mbr_bin DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/mbr.bin)

# Loader目标
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/loader.bin
    COMMAND nasm -I ${CMAKE_CURRENT_SOURCE_DIR}/include/
                 ${CMAKE_CURRENT_SOURCE_DIR}/loader.asm 
                 -o ${CMAKE_CURRENT_BINARY_DIR}/loader.bin
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/loader.asm
    COMMENT "Building Loader"
    VERBATIM
)

add_custom_target(loader_bin DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/loader.bin)

# 写入磁盘镜像的目标
add_disk_target(write_mbr ${CMAKE_CURRENT_BINARY_DIR}/mbr.bin 0 1)
add_disk_target(write_loader ${CMAKE_CURRENT_BINARY_DIR}/loader.bin 2 4)

# 添加依赖关系
add_dependencies(write_mbr mbr_bin)
add_dependencies(write_loader loader_bin)

# 组合运行配置: 同时写入 MBR 与 Loader
add_custom_target(boot
    COMMENT "Writing MBR and Loader to disk image"
)
add_dependencies(boot write_mbr write_loader)
