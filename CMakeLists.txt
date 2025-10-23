cmake_minimum_required(VERSION 4.0.1)

project(KernelOS
    LANGUAGES C ASM_NASM CXX
    VERSION 1.0.0
    DESCRIPTION "Custom Operating System Project"
)

# 设置编译器和标准
set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 26)

# 32位目标架构设置
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector")

# C++ 32位目标架构设置
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti -fno-threadsafe-statics")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-use-cxa-atexit -nostdlib++ -march=i386")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostdlib")


# NASM汇编器设置
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf32)
set(CMAKE_ASM_NASM_FLAGS "-f elf32")
set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> <INCLUDES> <FLAGS> -o <OBJECT> <SOURCE>")

# 输出目录设置
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 磁盘镜像文件路径
set(DISK_IMAGE "${CMAKE_SOURCE_DIR}/hd64M.img")

# 添加自定义函数用于创建磁盘镜像目标
function(add_disk_target target_name binary_file sector_start sector_count)
    add_custom_target(${target_name}
        COMMAND dd if=${binary_file} of=${DISK_IMAGE} bs=512 count=${sector_count} seek=${sector_start} conv=notrunc
        DEPENDS ${binary_file}
        COMMENT "Writing ${target_name} to disk image at sector ${sector_start}"
        VERBATIM
    )
endfunction()

# 添加子目录
add_subdirectory(boot)
add_subdirectory(kernel)
add_subdirectory(device)

# 创建完整构建目标
add_custom_target(osbuild
    DEPENDS write_mbr write_loader write_kernel write_program_no_arg
    COMMENT "Building complete OS image"
)
