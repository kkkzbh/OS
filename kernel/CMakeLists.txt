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
        system
)

add_library(string_impl STATIC
        src/string_impl.c
)

target_include_directories(string_impl
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/
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
        string_impl
        assert
)

add_library(system STATIC
        src/os.c
        src/os.asm
)

target_include_directories(system
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(system
        PRIVATE
        stdio
)

add_subdirectory(module)

set(DEVICE ${CMAKE_SOURCE_DIR}/device)

add_library(init
        init.cpp
)

target_include_directories(init
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/
)

target_link_libraries(init
        PRIVATE
        stdio
        interrupt
        time
        assert
        keyboard
        memory
        sync_alloc
        thread
        user
        ide
        filesystem
        console
        thread_init
)

add_library(kernel_module)

target_sources(kernel_module
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        kernel.cpp
)

target_link_libraries(kernel_module
        PRIVATE
        stdio
        interrupt
        console
        init
        thread
        user
        new
        std
        time
        stdfmt
)

add_library(os
        start.cpp
)

target_sources(os
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        main.cpp
        wexe.cpp
)

target_link_libraries(os
        PRIVATE
        kernel_module
        test_os
        stdio
)

# 创建内核可执行文件
add_executable(kernel
        main.c
        interrupt.asm
)

# 设置内核的包含目录
target_include_directories(kernel PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 链接stdio库到内核
target_link_libraries(kernel
        PRIVATE
        stdio
        os
)

# 设置编译选项 - 添加调试信息

target_compile_options(kernel PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-Os;-fdata-sections;-ffunction-sections>
        $<$<COMPILE_LANGUAGE:CXX>:-Os;-fdata-sections;-ffunction-sections>
        $<$<COMPILE_LANGUAGE:ASM>:-gdwarf-2>
)

# 设置链接器选项 - 修复入口点问题 (现代写法)
target_link_options(kernel PRIVATE
        "-nostartfiles"
        "-Wl,-Ttext,0xC0001500"
        "-Wl,-e,kkkzbh"
        "-Wl,--gc-sections"
)

add_library(kernel_utility INTERFACE)

target_include_directories(kernel_utility
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_custom_command(TARGET kernel POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} --strip-all $<TARGET_FILE:kernel> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/kernel_stripped
    COMMENT "Stripping debug symbols from kernel to create kernel_stripped"
    VERBATIM
)

add_disk_target(write_kernel ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/kernel_stripped 9 380)
add_dependencies(write_kernel kernel)

add_subdirectory(test)