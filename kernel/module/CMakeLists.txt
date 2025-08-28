

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
        thread/init.cppm
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

add_library(user)

target_sources(user
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        user/syscall.cpp
        user/syscall_init.cpp
        user/utility.cpp
        user/getpid.cpp
        user/write.cpp
        user/printf.cpp
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