

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
        memory/utilty.cppm
)

target_link_libraries(memory
        PRIVATE
        utility
        stdio
        string
        assert
)

add_library(sync_alloc
        thread/sync/switch.asm
)

target_sources(sync_alloc
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        thread/sync/task.cpp
        thread/sync/list.cppm
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
        user
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

add_library(user)

target_sources(user
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        user/syscall.cpp
        user/syscall_init.cpp
        user/utility.cpp
        user/getpid.cpp
        user/printf.cpp
        user/fork.cpp
        user/ps.cpp
        user/exec.cpp
)

target_link_libraries(user
        PRIVATE
        utility
        sync_alloc
        stdio
        string
        assert
        format
        console
        filesystem
        thread
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
        assert
        string
)

add_library(std)

target_sources(std
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        std/bit.cpp
        std/string.cpp
        std/vector.cpp
        std/bufer.cpp
        std/scope.cpp
)

target_link_libraries(std
        PRIVATE
        utility
        string
        optional
        sysnew
        PUBLIC
        std_algorithm
        INTERFACE
        std_array
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

add_library(stdfmt)

target_sources(stdfmt
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        std/format/array.cpp
        std/format/string.cpp
        std/format/vector.cpp
)

target_link_libraries(stdfmt
        PRIVATE
        std
        format
        utility
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

add_library(filesystem)

target_sources(filesystem
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        filesystem/dir.cpp
        filesystem/inode.cpp
        filesystem/fs.cpp
        filesystem/init.cpp
        filesystem/utility.cpp
        filesystem/file_manager.cpp
        filesystem/file_struct.cpp
        filesystem/file.cpp
        filesystem/dir_struct.cpp
        filesystem/path.cpp
        filesystem/path_struct.cpp
        filesystem/sysfunc.cpp
        filesystem/inode_free.cpp
        filesystem/inode_structure.cpp
        filesystem/stat_struct.cpp
)

target_link_libraries(filesystem
        PRIVATE
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
)

add_subdirectory(program)