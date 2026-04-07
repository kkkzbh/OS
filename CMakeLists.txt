cmake_minimum_required(VERSION 4.0.1)

project(KernelOS
    LANGUAGES C ASM_NASM CXX
    VERSION 1.0.0
    DESCRIPTION "Custom Operating System Project"
)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

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

# ============== Bochs 配置 ==============
set(BOCHS_NODEBUG "/usr/local/bochs/bin/bochs")
set(BOCHS_GDB "/usr/local/bochs-gdb/bin/bochs")
find_program(QEMU_SYSTEM_I386 qemu-system-i386)

# Bochs 普通运行
add_custom_target(bochs
    COMMAND ${BOCHS_NODEBUG} -qf bochsrc.disk
    DEPENDS osbuild
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Starting Bochs..."
    USES_TERMINAL
)

# Bochs GDB 调试
add_custom_target(bochs-gdb
    COMMAND ${BOCHS_GDB} -qf bochsrc-gdb.disk
    DEPENDS osbuild
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Starting Bochs with GDB support..."
    USES_TERMINAL
)

add_custom_target(bochs-smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/bochs_debug.py --bochs-binary ${BOCHS_NODEBUG} --bochs-config bochsrc.disk
    DEPENDS osbuild
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running Bochs shell smoke test..."
    USES_TERMINAL
)

if(QEMU_SYSTEM_I386)
add_custom_target(qemu
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/qemu_debug.py run --build-dir ${CMAKE_BINARY_DIR} --qemu-binary ${QEMU_SYSTEM_I386}
    DEPENDS osbuild
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Starting QEMU..."
    USES_TERMINAL
)

add_custom_target(qemu-gdb
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/qemu_debug.py run --build-dir ${CMAKE_BINARY_DIR} --qemu-binary ${QEMU_SYSTEM_I386} --gdb 1234 --paused
    DEPENDS osbuild
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Starting QEMU with GDB support on tcp::1234..."
    USES_TERMINAL
)

add_custom_target(qemu-smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/qemu_debug.py smoke --build-dir ${CMAKE_BINARY_DIR} --qemu-binary ${QEMU_SYSTEM_I386}
    DEPENDS osbuild
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running headless QEMU shell smoke test..."
    USES_TERMINAL
)
endif()
