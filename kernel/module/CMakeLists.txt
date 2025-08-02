

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
)

target_link_libraries(thread
        PRIVATE
        utility
        memory
        string
        stdio
)

add_library(list)

target_sources(list
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        list.cppm
)

target_link_libraries(list
        PRIVATE
        interrupt
)