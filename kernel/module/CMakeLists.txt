

add_library(optional)

target_sources(optional
        PUBLIC
        FILE_SET CXX_MODULES
        FILES optional.cppm
)

target_link_libraries(optional
        PRIVATE
        stdio
        utility
)

add_library(utility)

target_sources(utility
        PUBLIC
        FILE_SET CXX_MODULES
        FILES utility.cppm
)

target_include_directories(utility
        PRIVATE
        ${KERNEL}/include/
)

add_library(bitmap)

target_sources(bitmap
        PUBLIC
        FILE_SET CXX_MODULES
        FILES bitmap.cppm
)

target_link_libraries(bitmap
        PRIVATE
        stdio
        string
        interrupt
        assert
        PUBLIC
        optional
)

add_library(memory)

target_sources(memory
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        memory/memory.cppm
        memory/pgtable.cppm
)

target_link_libraries(memory
        PRIVATE
        utility
        stdio
        string
        assert
        memory_utility
)

add_library(memory_utility)

target_sources(memory_utility
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        memory/utilty.cppm
)

target_link_libraries(memory_utility
        PRIVATE
        utility
)

add_library(list_node)

target_sources(list_node
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        thread/sync/list_node.cppm
)

add_library(list)

target_sources(list
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        thread/sync/list.cppm
)

target_link_libraries(list
        PUBLIC
        list_node
        PRIVATE
        utility
        kernel_utility
)

add_library(sync_alloc
        thread/sync/switch.asm
)

target_sources(sync_alloc
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        thread/sync/task.cpp
        thread/sync/semaphore.cppm
        thread/sync/mutex.cppm
        thread/sync/lock_guard.cpp
        thread/sync/schedule.cpp
        thread/sync/tss.cpp
        memory/alloc/alloc.cpp
        memory/alloc/init.cppm
        memory/alloc/pool.cppm
        memory/alloc/arena.cpp
        memory/alloc/malloc.cpp
        memory/alloc/free.cpp
)

target_include_directories(sync_alloc
        PUBLIC
        thread/sync/
)

target_link_libraries(sync_alloc
        PRIVATE
        list
        utility
        stdio
        interrupt
        assert
        bitmap
        string
        kernel_utility
        memory
        std_array
)

add_library(new
        memory/alloc/new.cpp
)

target_link_libraries(new
        PRIVATE
        syscall
        utility
)

add_library(sysnew
        memory/alloc/sysnew.cpp
)

target_link_libraries(sysnew
        PRIVATE
        sync_alloc
        utility
)

add_library(thread)

target_sources(thread
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        thread/thread.cppm
        thread/utility.cppm
        thread/stack.cppm
        thread/exec.cppm
        thread/process.cpp
)

target_link_libraries(thread
        PRIVATE
        utility
        string
        stdio
        interrupt
        assert
        kernel_utility
        sync_alloc
        thread_pid
        memory_utility
)

add_library(thread_pid)

target_sources(thread_pid
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        thread/pid.cpp
)

target_link_libraries(thread_pid PRIVATE
        utility
        std_array
        sync_alloc
)

add_library(thread_init)

target_sources(thread_init
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        thread/init.cppm
)

target_link_libraries(thread_init PRIVATE
        stdio
        thread
        console
        user
        shell
)

add_library(syscall)

target_sources(syscall
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        user/syscall.cpp
        user/utility.cpp
        user/printf.cpp
)

target_link_libraries(syscall
        PRIVATE
        utility
        std_string
        filesystem_types
        kernel_utility
        format
)

add_library(user)

target_sources(user
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        user/syscall_init.cpp
        user/getpid.cpp
        user/fork.cpp
        user/ps.cpp
        user/exec.cpp
)

target_link_libraries(user
        PRIVATE
        syscall
        utility
        sync_alloc
        stdio
        string
        assert
        format
        console
        filesystem
        thread
        thread_pid
        wait_exit
)

add_library(format)

target_sources(format
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        user/format.cpp
)

target_link_libraries(format
        PRIVATE
        utility
        std_string
)

# std_core: 无分配，最轻量
add_library(std_core)

target_sources(std_core
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        std/bit.cpp
        std/scope.cpp
)

target_link_libraries(std_core
        PRIVATE
        utility
)

# std_string: 无分配，string_view 和 buffer
add_library(std_string)

target_sources(std_string
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        std/string.cpp
        std/buffer.cpp
)

target_link_libraries(std_string
        PRIVATE
        std_core
        std_algorithm
        optional
        kernel_utility
)

# std_vector: 需要分配
add_library(std_vector)

target_sources(std_vector
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        std/vector.cpp
)

target_link_libraries(std_vector
        PRIVATE
        utility
        std_algorithm
        INTERFACE
        sysnew
)

# user_std_vector: 用户态 vector（使用 new 库的 syscall 分配）
add_library(user_std_vector)

target_sources(user_std_vector
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        std/vector.cpp
)

target_link_libraries(user_std_vector
        PRIVATE
        utility
        std_algorithm
)

# std: 聚合库（内核态用）
add_library(std INTERFACE)

target_link_libraries(std
        INTERFACE
        std_core
        std_string
        std_array
        std_vector
        stdfmt
)

add_library(std_algorithm)

target_sources(std_algorithm
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        std/algorithm.cpp
)

target_link_libraries(std_algorithm
        PRIVATE
        utility
        optional
        ref
)

add_library(std_array)

target_sources(std_array
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        std/array.cpp
)

target_link_libraries(std_array
        PRIVATE
        utility
        std_algorithm
)

# 格式化库 - 对应容器层
add_library(std_array_format)

target_sources(std_array_format
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        std/format/array.cpp
)

target_link_libraries(std_array_format
        PRIVATE
        std_array
        format
)

add_library(std_string_format)

target_sources(std_string_format
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        std/format/string.cpp
)

target_link_libraries(std_string_format
        PRIVATE
        std_string
        format
)

add_library(std_vector_format)

target_sources(std_vector_format
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        std/format/vector.cpp
)

target_link_libraries(std_vector_format
        PRIVATE
        std_vector
        format
)

# user_std_vector_format: 用户态 vector 格式化
add_library(user_std_vector_format)

target_sources(user_std_vector_format
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        std/format/vector.cpp
)

target_link_libraries(user_std_vector_format
        PRIVATE
        user_std_vector
        format
)

# stdfmt: 聚合库（内核态用）
add_library(stdfmt INTERFACE)

target_link_libraries(stdfmt
        INTERFACE
        std_array_format
        std_string_format
        std_vector_format
)

add_library(sleep)

target_sources(sleep
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        thread/sync/sleep.cpp
)

target_link_libraries(sleep
        PRIVATE
        sync_alloc
        utility
        PUBLIC
        time
)

add_library(filesystem_types)

target_sources(filesystem_types
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        filesystem/flags.cpp
        filesystem/utility.cpp
        filesystem/stat_struct.cpp
        filesystem/dir_struct.cpp
        filesystem/inode_structure.cpp
        filesystem/file_struct.cpp
        filesystem/path_struct.cpp
)

target_link_libraries(filesystem_types
        PRIVATE
        utility
        list_node
        std_array
)

add_library(filesystem)

target_sources(filesystem
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        filesystem/dir.cpp
        filesystem/inode.cpp
        filesystem/fs.cpp
        filesystem/init.cpp
        filesystem/file_manager.cpp
        filesystem/file.cpp
        filesystem/path.cpp
        filesystem/sysfunc.cpp
        filesystem/inode_free.cpp
)

target_link_libraries(filesystem
        PRIVATE
        filesystem_types
        utility
        std
        sync_alloc
        ide
        console
        sysnew
        assert
        string
        super_block
        interrupt
        optional
        format
        keyboard
        sysnew
)


add_library(super_block)

target_sources(super_block
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        filesystem/super_block.cpp
)

target_link_libraries(super_block
        PRIVATE
        utility
        std
)

add_library(ref)

target_sources(ref
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        ref.cpp
)

target_link_libraries(ref
        PRIVATE
        utility
        optional
)


add_library(shell)

target_sources(shell
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        shell/shell.cpp
        shell/builtin.cpp
)

target_link_libraries(shell
        PRIVATE
        std_array
        stdfmt
        assert
        filesystem
        user
        wait_exit
)

add_subdirectory(program)

add_library(wait_exit)

target_sources(wait_exit
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        user/wait_exit.cpp
        user/wait.cpp
        user/exit.cpp
)

target_link_libraries(wait_exit
        PRIVATE
        sync_alloc
        std_algorithm
        memory
        list_node
        utility
        assert
        thread
        filesystem
)

