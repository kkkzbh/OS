set(BOOT_TEST_ARTIFACTS_DIR "${CMAKE_BINARY_DIR}/test/artifacts")

knix_add_boot_test(boot.image.layout
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/tests/boot/check_image_layout.py
        --cmake-command ${CMAKE_COMMAND}
        --build-dir ${CMAKE_BINARY_DIR}
        --source-dir ${CMAKE_SOURCE_DIR}
    FIXTURES_SETUP BootImage
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

if(QEMU_SYSTEM_I386)
    knix_add_boot_test(boot.qemu.milestones
        QEMU
        COMMAND
            ${Python3_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/scripts/qemu_debug.py
            test
            --scenario boot_milestones
            --build-dir ${CMAKE_BINARY_DIR}
            --qemu-binary ${QEMU_SYSTEM_I386}
            --artifacts-dir ${BOOT_TEST_ARTIFACTS_DIR}/boot.qemu.milestones
        FIXTURES_REQUIRED BootImage
        RESOURCE_LOCK qemu_boot
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    knix_add_boot_test(boot.qemu.prompt_clean
        QEMU
        COMMAND
            ${Python3_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/scripts/qemu_debug.py
            test
            --scenario boot_prompt_clean
            --build-dir ${CMAKE_BINARY_DIR}
            --qemu-binary ${QEMU_SYSTEM_I386}
            --artifacts-dir ${BOOT_TEST_ARTIFACTS_DIR}/boot.qemu.prompt_clean
        FIXTURES_REQUIRED BootImage
        RESOURCE_LOCK qemu_boot
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()
