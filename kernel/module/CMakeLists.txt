

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
        user/format.cpp
        user/printf.cpp
)

target_link_libraries(user
        PRIVATE
        utility
        sync_alloc
        stdio
        console
        string
        assert
)

add_library(stringm)

target_sources(stringm
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        string.cpp
)