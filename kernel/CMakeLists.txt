# Kernel组件构建配置

SET(KERNEL ${CMAKE_SOURCE_DIR}/kernel/)

# 创建stdio库
add_library(stdio STATIC
        src/stdio.asm
)

# 设置stdio库的汇编包含目录
target_include_directories(stdio
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_library(assert STATIC
        src/assert.c
)

target_include_directories(assert
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(assert
        PRIVATE
        interrupt
)

add_library(interrupt STATIC
        interrupt.c
)

target_include_directories(interrupt
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/
)

target_link_libraries(interrupt
        PRIVATE
        stdio
)

add_library(string STATIC
        src/string.c
)

target_include_directories(string
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/
)

target_link_libraries(string
        PRIVATE
        assert
)

add_subdirectory(module)

set(DEVICE ${CMAKE_SOURCE_DIR}/device)

# 创建内核可执行文件
add_executable(kernel
        main.c
        kernel.asm
        init.cpp
        ${DEVICE}/time.c
        main.cpp
)

# 设置内核的包含目录
target_include_directories(kernel PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${DEVICE}/
)

# 链接stdio库到内核
target_link_libraries(kernel
        PRIVATE
        stdio
        interrupt
        assert
        memory
        thread
)

# 设置编译选项 - 添加调试信息
target_compile_options(kernel PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-g -O0>
    $<$<COMPILE_LANGUAGE:CXX>:-g -O0>
    $<$<COMPILE_LANGUAGE:ASM>:-g -F dwarf>
)

# 设置链接器选项 - 修复入口点问题 (现代写法)
target_link_options(kernel PRIVATE
    "-nostartfiles"
    "-Wl,-Ttext,0xC0001500"
    "-Wl,-e,kkkzbh"
)

add_disk_target(write_kernel $<TARGET_FILE:kernel> 9 200)
add_dependencies(write_kernel kernel)