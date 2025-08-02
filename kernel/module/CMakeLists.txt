

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

add_library(bitmap)

target_sources(bitmap
        PUBLIC
        FILE_SET CXX_MODULES
        FILES bitmap.cppm
)

target_include_directories(bitmap
        PRIVATE
        ${KERNEL}/include/
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
        memory.cppm
)

target_include_directories(memory
        PRIVATE
        ${KERNEL}/include/
)

target_link_libraries(memory
        PRIVATE
        bitmap
)