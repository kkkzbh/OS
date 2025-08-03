

add_library(time
        time.c
        time.cpp
)

target_include_directories(time
        PRIVATE
        ${KERNEL}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(time
        PRIVATE
        stdio
        utility
        thread
        assert
        interrupt
)