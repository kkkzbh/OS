

add_library(time
        time/time.c
        time/time.cpp
)

target_include_directories(time
        PRIVATE
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
        PUBLIC
        kernel_utility
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
        string
        format
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

add_library(block_device)

target_sources(block_device
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        block/device.cpp
        block/partition.cpp
)

target_link_libraries(block_device
        PRIVATE
        utility
        sync_alloc
        bitmap
        std
        console
        assert
        format
        kernel_utility
        super_block
)

add_library(pci)

target_sources(pci
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        pci/pci.cpp
        pci/mmio.cpp
)

target_link_libraries(pci
        PRIVATE
        utility
        sync_alloc
        memory
        assert
)

add_library(ata_pio)

target_sources(ata_pio
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        ata_pio/ata_pio.cpp
        ata_pio/init.cpp
        ata_pio/intr.cpp
        ata_pio/regression.cpp
)

target_link_libraries(ata_pio
        PRIVATE
        block_device
        utility
        sync_alloc
        bitmap
        std
        console
        assert
        format
        kernel_utility
        sleep
        interrupt
        sysnew
        string
        super_block
)

add_library(ahci)

target_sources(ahci
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        ahci/ahci.cpp
)

target_link_libraries(ahci
        PRIVATE
        block_device
        pci
        utility
        sync_alloc
        std
        console
        assert
        format
        kernel_utility
        string
        memory
)

add_library(storage)

target_sources(storage
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        storage/init.cpp
        storage/regression.cpp
)

target_link_libraries(storage
        PRIVATE
        ahci
        ata_pio
        block_device
        utility
        std
        console
        assert
        format
        string
        kernel_utility
        sleep
        interrupt
)
