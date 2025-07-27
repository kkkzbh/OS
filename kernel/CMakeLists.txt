# Kernel组件构建配置

# 创建stdio库
add_library(stdio STATIC
    src/stdio.asm
)

# 设置stdio库的汇编包含目录
target_include_directories(stdio PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 创建内核可执行文件
add_executable(kernel
        main.c
        interrupt.c
        kernel.asm
        init.c
)

# 设置内核的包含目录
target_include_directories(kernel PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 链接stdio库到内核
target_link_libraries(kernel PRIVATE stdio)

# 设置链接器选项 - 修复入口点问题 (现代写法)
target_link_options(kernel PRIVATE
    "-nostartfiles"
    "-Wl,-Ttext,0xC0001500"
    "-Wl,-e,kkkzbh"
)

add_disk_target(write_kernel $<TARGET_FILE:kernel> 9 200)
add_dependencies(write_kernel kernel)