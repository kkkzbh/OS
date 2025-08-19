

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
        sync_alloc
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
        stdio
        sync_alloc
        format
        string
)

add_library(keyboard
        keyboard/keyboard.c
)

target_sources(keyboard
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        keyboard/iobuf.cpp
)

target_include_directories(keyboard
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/keyboard
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(keyboard
        PRIVATE
        kernel_utility
        stdio
        interrupt
        ioqueue
)

add_library(ioqueue)

target_sources(ioqueue
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        ioqueue/ioqueue.cpp
)

target_link_libraries(ioqueue
        PRIVATE
        sync_alloc
        utility
        stdio
        interrupt
        assert
)

add_library(ide)

target_sources(ide
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        ide/ide.cpp
        ide/init.cpp
)

target_link_libraries(ide
        PRIVATE
        utility
        sync_alloc
        bitmap
        std
        console
        assert
        user
        sync_alloc
)