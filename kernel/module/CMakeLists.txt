

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

add_library(thread)

target_sources(thread
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        thread/thread.cppm
        thread/utility.cppm
        thread/stack.cppm
        thread/exec.cppm
        thread/list.cppm
        thread/init.cppm
        thread/sync/semaphore.cppm
        thread/sync/mutux.cppm
)

target_sources(thread
        PRIVATE
        thread/switch.asm
)

target_include_directories(thread
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/thread/
        ${KERNEL}/include/
)

target_link_libraries(thread
        PRIVATE
        utility
        memory
        string
        stdio
        interrupt
        assert
)
