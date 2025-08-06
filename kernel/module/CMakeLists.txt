

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
        memory/init.cppm
        memory/pgtable.cppm
        memory/utilty.cppm
        memory/pool.cppm
)

target_link_libraries(memory
        PRIVATE
        bitmap
        utility
        stdio
        string
        assert
)

add_library(sync)

target_sources(sync
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        thread/sync/task.cpp
        thread/sync/sync.cpp
        thread/sync/list.cppm
        thread/sync/semaphore.cppm
        thread/sync/mutex.cppm
        thread/sync/lock_guard.cpp
        thread/sync/execution.cpp
)

target_sources(sync
        PRIVATE
        thread/sync/switch.asm
)

target_include_directories(sync
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/thread/sync/
)

target_link_libraries(sync
        PRIVATE
        utility
        stdio
        interrupt
        assert
        memory
)

add_library(alloc)

target_sources(alloc
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        memory/alloc/alloc.cpp
)

target_link_libraries(alloc
        PRIVATE
        memory
        assert
        string
        sync
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
)

target_link_libraries(thread
        PRIVATE
        utility
        string
        stdio
        interrupt
        assert
        kernel_utility
        alloc
)