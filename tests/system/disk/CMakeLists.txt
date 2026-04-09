set(DISK_TEST_ARTIFACT_ROOT "${CMAKE_BINARY_DIR}/test/artifacts")
set(DISK_TEST_IMAGE_ROOT "${CMAKE_BINARY_DIR}/test/images/disk")
set(DISK_SUITE_IMAGE_DIR "${DISK_TEST_IMAGE_ROOT}/suite")
set(DISK_PREPARE_SCRIPT "${CMAKE_SOURCE_DIR}/tests/system/disk/scripts/prepare_disk_images.py")
set(DISK_QEMU_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/qemu_debug.py")
set(DISK_BOCHS_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/bochs_debug.py")

knix_add_system_test(disk.image.base
    COMMAND
        ${Python3_EXECUTABLE}
        ${DISK_PREPARE_SCRIPT}
        copy
        --source-dir ${CMAKE_SOURCE_DIR}
        --image-root ${DISK_TEST_IMAGE_ROOT}
    FIXTURES_SETUP DiskImageBase
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    LABELS fixture
)

knix_add_system_test(disk.image.seeded
    COMMAND
        ${Python3_EXECUTABLE}
        ${DISK_PREPARE_SCRIPT}
        seed
        --cmake-command ${CMAKE_COMMAND}
        --build-dir ${CMAKE_BINARY_DIR}
        --source-dir ${CMAKE_SOURCE_DIR}
        --image-root ${DISK_TEST_IMAGE_ROOT}
    FIXTURES_REQUIRED DiskImageBase
    FIXTURES_SETUP DiskImages
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    LABELS fixture
)

if(QEMU_SYSTEM_I386)
knix_add_system_test(disk.qemu.read_sector
    QEMU
    COMMAND
        ${Python3_EXECUTABLE}
        ${DISK_QEMU_SCRIPT}
        test
        --scenario disk_read_sector
        --build-dir ${CMAKE_BINARY_DIR}
        --qemu-binary ${QEMU_SYSTEM_I386}
        --artifacts-dir ${DISK_TEST_ARTIFACT_ROOT}/disk.qemu.read_sector
        --os-image ${DISK_SUITE_IMAGE_DIR}/hd64M.img
        --data-image ${DISK_SUITE_IMAGE_DIR}/hd80M.img
        --clone-images
        --no-snapshot
    FIXTURES_REQUIRED DiskImages
    RESOURCE_LOCK qemu_disk
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    LABELS disk regression
)

knix_add_system_test(disk.qemu.cross_sector_read
    QEMU
    COMMAND
        ${Python3_EXECUTABLE}
        ${DISK_QEMU_SCRIPT}
        test
        --scenario disk_cross_sector_read
        --build-dir ${CMAKE_BINARY_DIR}
        --qemu-binary ${QEMU_SYSTEM_I386}
        --artifacts-dir ${DISK_TEST_ARTIFACT_ROOT}/disk.qemu.cross_sector_read
        --os-image ${DISK_SUITE_IMAGE_DIR}/hd64M.img
        --data-image ${DISK_SUITE_IMAGE_DIR}/hd80M.img
        --clone-images
        --no-snapshot
    FIXTURES_REQUIRED DiskImages
    RESOURCE_LOCK qemu_disk
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    LABELS disk regression
)

knix_add_system_test(disk.qemu.read_after_write
    QEMU
    COMMAND
        ${Python3_EXECUTABLE}
        ${DISK_QEMU_SCRIPT}
        test
        --scenario disk_read_after_write
        --build-dir ${CMAKE_BINARY_DIR}
        --qemu-binary ${QEMU_SYSTEM_I386}
        --artifacts-dir ${DISK_TEST_ARTIFACT_ROOT}/disk.qemu.read_after_write
        --os-image ${DISK_SUITE_IMAGE_DIR}/hd64M.img
        --data-image ${DISK_SUITE_IMAGE_DIR}/hd80M.img
        --clone-images
        --no-snapshot
    FIXTURES_REQUIRED DiskImages
    RESOURCE_LOCK qemu_disk
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    LABELS disk regression
)

knix_add_system_test(disk.qemu.partition_table_scan
    QEMU
    COMMAND
        ${Python3_EXECUTABLE}
        ${DISK_QEMU_SCRIPT}
        test
        --scenario disk_partition_table_scan
        --build-dir ${CMAKE_BINARY_DIR}
        --qemu-binary ${QEMU_SYSTEM_I386}
        --artifacts-dir ${DISK_TEST_ARTIFACT_ROOT}/disk.qemu.partition_table_scan
        --os-image ${DISK_SUITE_IMAGE_DIR}/hd64M.img
        --data-image ${DISK_SUITE_IMAGE_DIR}/hd80M.img
        --clone-images
        --no-snapshot
    FIXTURES_REQUIRED DiskImages
    RESOURCE_LOCK qemu_disk
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    LABELS disk regression
)
endif()

if(BOCHS_GDB)
knix_add_system_test(disk.bochs.read_sector
    BOCHS
    COMMAND
        ${Python3_EXECUTABLE}
        ${DISK_BOCHS_SCRIPT}
        test
        --scenario disk_read_sector
        --artifacts-dir ${DISK_TEST_ARTIFACT_ROOT}/disk.bochs.read_sector
        --bochs-binary ${BOCHS_GDB}
        --bochs-config ${CMAKE_SOURCE_DIR}/bochsrc-gdb.disk
        --kernel ${CMAKE_BINARY_DIR}/bin/kernel
        --os-image ${DISK_SUITE_IMAGE_DIR}/hd64M.img
        --data-image ${DISK_SUITE_IMAGE_DIR}/hd80M.img
        --clone-images
    FIXTURES_REQUIRED DiskImages
    RESOURCE_LOCK bochs_disk
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    LABELS disk regression
)

knix_add_system_test(disk.bochs.cross_sector_read
    BOCHS
    COMMAND
        ${Python3_EXECUTABLE}
        ${DISK_BOCHS_SCRIPT}
        test
        --scenario disk_cross_sector_read
        --artifacts-dir ${DISK_TEST_ARTIFACT_ROOT}/disk.bochs.cross_sector_read
        --bochs-binary ${BOCHS_GDB}
        --bochs-config ${CMAKE_SOURCE_DIR}/bochsrc-gdb.disk
        --kernel ${CMAKE_BINARY_DIR}/bin/kernel
        --os-image ${DISK_SUITE_IMAGE_DIR}/hd64M.img
        --data-image ${DISK_SUITE_IMAGE_DIR}/hd80M.img
        --clone-images
    FIXTURES_REQUIRED DiskImages
    RESOURCE_LOCK bochs_disk
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    LABELS disk regression
)

knix_add_system_test(disk.bochs.read_after_write
    BOCHS
    COMMAND
        ${Python3_EXECUTABLE}
        ${DISK_BOCHS_SCRIPT}
        test
        --scenario disk_read_after_write
        --artifacts-dir ${DISK_TEST_ARTIFACT_ROOT}/disk.bochs.read_after_write
        --bochs-binary ${BOCHS_GDB}
        --bochs-config ${CMAKE_SOURCE_DIR}/bochsrc-gdb.disk
        --kernel ${CMAKE_BINARY_DIR}/bin/kernel
        --os-image ${DISK_SUITE_IMAGE_DIR}/hd64M.img
        --data-image ${DISK_SUITE_IMAGE_DIR}/hd80M.img
        --clone-images
    FIXTURES_REQUIRED DiskImages
    RESOURCE_LOCK bochs_disk
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    LABELS disk regression
)

knix_add_system_test(disk.bochs.partition_table_scan
    BOCHS
    COMMAND
        ${Python3_EXECUTABLE}
        ${DISK_BOCHS_SCRIPT}
        test
        --scenario disk_partition_table_scan
        --artifacts-dir ${DISK_TEST_ARTIFACT_ROOT}/disk.bochs.partition_table_scan
        --bochs-binary ${BOCHS_GDB}
        --bochs-config ${CMAKE_SOURCE_DIR}/bochsrc-gdb.disk
        --kernel ${CMAKE_BINARY_DIR}/bin/kernel
        --os-image ${DISK_SUITE_IMAGE_DIR}/hd64M.img
        --data-image ${DISK_SUITE_IMAGE_DIR}/hd80M.img
        --clone-images
    FIXTURES_REQUIRED DiskImages
    RESOURCE_LOCK bochs_disk
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    LABELS disk regression
)
endif()
