

add_library(time
        time/time.c
        time/time.cpp
)

target_include_directories(time
        PRIVATE
        ${KERNEL}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/time/
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

add_library(console)

target_sources(console
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        console/console.cpp
)

target_link_libraries(console
        PRIVATE
        utility
        thread
)