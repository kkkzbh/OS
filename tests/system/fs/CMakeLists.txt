set(FS_TEST_ARTIFACT_ROOT "${CMAKE_BINARY_DIR}/test/artifacts")
set(FS_TEST_IMAGE_ROOT "${CMAKE_BINARY_DIR}/test/images/fs")
set(FS_SUITE_IMAGE_DIR "${FS_TEST_IMAGE_ROOT}/suite")
set(FS_PERSIST_FILE_IMAGE_DIR "${FS_TEST_IMAGE_ROOT}/persistence-file")
set(FS_PERSIST_DIR_IMAGE_DIR "${FS_TEST_IMAGE_ROOT}/persistence-directory")
set(FS_PREPARE_SCRIPT "${CMAKE_SOURCE_DIR}/tests/system/fs/scripts/prepare_fs_images.py")
set(FS_REQUIRE_SUCCESS_SCRIPT "${CMAKE_SOURCE_DIR}/tests/system/fs/scripts/require_success.py")
set(FS_QEMU_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/qemu_debug.py")

knix_add_system_test(fs.image.base
    COMMAND
        ${Python3_EXECUTABLE}
        ${FS_PREPARE_SCRIPT}
        copy
        --source-dir ${CMAKE_SOURCE_DIR}
        --image-root ${FS_TEST_IMAGE_ROOT}
    FIXTURES_SETUP FsImageBase
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    LABELS fs fixture
)

knix_add_system_test(fs.image.seeded
    COMMAND
        ${Python3_EXECUTABLE}
        ${FS_PREPARE_SCRIPT}
        seed
        --cmake-command ${CMAKE_COMMAND}
        --build-dir ${CMAKE_BINARY_DIR}
        --source-dir ${CMAKE_SOURCE_DIR}
        --image-root ${FS_TEST_IMAGE_ROOT}
    FIXTURES_REQUIRED FsImageBase
    FIXTURES_SETUP FsImages
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    LABELS fs fixture
)

if(QEMU_SYSTEM_I386)
knix_add_system_test(fs.shell.cwd
    QEMU
    COMMAND
        ${Python3_EXECUTABLE}
        ${FS_QEMU_SCRIPT}
        test
        --scenario shell_fs_cwd
        --build-dir ${CMAKE_BINARY_DIR}
        --qemu-binary ${QEMU_SYSTEM_I386}
        --artifacts-dir ${FS_TEST_ARTIFACT_ROOT}/fs.shell.cwd
        --os-image ${FS_SUITE_IMAGE_DIR}/hd64M.img
        --data-image ${FS_SUITE_IMAGE_DIR}/hd80M.img
    FIXTURES_REQUIRED FsImages
    RESOURCE_LOCK qemu_fs
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    LABELS fs shell
)

knix_add_system_test(fs.shell.mkdir_rmdir
    QEMU
    COMMAND
        ${Python3_EXECUTABLE}
        ${FS_QEMU_SCRIPT}
        test
        --scenario shell_fs_mkdir_rmdir
        --build-dir ${CMAKE_BINARY_DIR}
        --qemu-binary ${QEMU_SYSTEM_I386}
        --artifacts-dir ${FS_TEST_ARTIFACT_ROOT}/fs.shell.mkdir_rmdir
        --os-image ${FS_SUITE_IMAGE_DIR}/hd64M.img
        --data-image ${FS_SUITE_IMAGE_DIR}/hd80M.img
    FIXTURES_REQUIRED FsImages
    RESOURCE_LOCK qemu_fs
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    LABELS fs shell
)

knix_add_system_test(fs.shell.ls
    QEMU
    COMMAND
        ${Python3_EXECUTABLE}
        ${FS_QEMU_SCRIPT}
        test
        --scenario shell_fs_ls
        --build-dir ${CMAKE_BINARY_DIR}
        --qemu-binary ${QEMU_SYSTEM_I386}
        --artifacts-dir ${FS_TEST_ARTIFACT_ROOT}/fs.shell.ls
        --os-image ${FS_SUITE_IMAGE_DIR}/hd64M.img
        --data-image ${FS_SUITE_IMAGE_DIR}/hd80M.img
    FIXTURES_REQUIRED FsImages
    RESOURCE_LOCK qemu_fs
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    LABELS fs shell
)

knix_add_system_test(fs.shell.negative
    QEMU
    COMMAND
        ${Python3_EXECUTABLE}
        ${FS_QEMU_SCRIPT}
        test
        --scenario shell_fs_negative
        --build-dir ${CMAKE_BINARY_DIR}
        --qemu-binary ${QEMU_SYSTEM_I386}
        --artifacts-dir ${FS_TEST_ARTIFACT_ROOT}/fs.shell.negative
        --os-image ${FS_SUITE_IMAGE_DIR}/hd64M.img
        --data-image ${FS_SUITE_IMAGE_DIR}/hd80M.img
    FIXTURES_REQUIRED FsImages
    RESOURCE_LOCK qemu_fs
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    LABELS fs shell
)

knix_add_system_test(fs.syscall.file_basic
    QEMU
    COMMAND
        ${Python3_EXECUTABLE}
        ${FS_QEMU_SCRIPT}
        test
        --scenario runner_fs_basic
        --build-dir ${CMAKE_BINARY_DIR}
        --qemu-binary ${QEMU_SYSTEM_I386}
        --artifacts-dir ${FS_TEST_ARTIFACT_ROOT}/fs.syscall.file_basic
        --os-image ${FS_SUITE_IMAGE_DIR}/hd64M.img
        --data-image ${FS_SUITE_IMAGE_DIR}/hd80M.img
    FIXTURES_REQUIRED FsImages
    RESOURCE_LOCK qemu_fs
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    LABELS fs syscall
)

knix_add_system_test(fs.syscall.file_offset
    QEMU
    COMMAND
        ${Python3_EXECUTABLE}
        ${FS_QEMU_SCRIPT}
        test
        --scenario runner_fs_file_offset
        --build-dir ${CMAKE_BINARY_DIR}
        --qemu-binary ${QEMU_SYSTEM_I386}
        --artifacts-dir ${FS_TEST_ARTIFACT_ROOT}/fs.syscall.file_offset
        --os-image ${FS_SUITE_IMAGE_DIR}/hd64M.img
        --data-image ${FS_SUITE_IMAGE_DIR}/hd80M.img
    FIXTURES_REQUIRED FsImages
    RESOURCE_LOCK qemu_fs
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    LABELS fs syscall
)

knix_add_system_test(fs.syscall.directory_basic
    QEMU
    COMMAND
        ${Python3_EXECUTABLE}
        ${FS_QEMU_SCRIPT}
        test
        --scenario runner_fs_directory_basic
        --build-dir ${CMAKE_BINARY_DIR}
        --qemu-binary ${QEMU_SYSTEM_I386}
        --artifacts-dir ${FS_TEST_ARTIFACT_ROOT}/fs.syscall.directory_basic
        --os-image ${FS_SUITE_IMAGE_DIR}/hd64M.img
        --data-image ${FS_SUITE_IMAGE_DIR}/hd80M.img
    FIXTURES_REQUIRED FsImages
    RESOURCE_LOCK qemu_fs
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    LABELS fs syscall
)

knix_add_system_test(fs.syscall.directory_iteration
    QEMU
    COMMAND
        ${Python3_EXECUTABLE}
        ${FS_QEMU_SCRIPT}
        test
        --scenario runner_fs_directory_iteration
        --build-dir ${CMAKE_BINARY_DIR}
        --qemu-binary ${QEMU_SYSTEM_I386}
        --artifacts-dir ${FS_TEST_ARTIFACT_ROOT}/fs.syscall.directory_iteration
        --os-image ${FS_SUITE_IMAGE_DIR}/hd64M.img
        --data-image ${FS_SUITE_IMAGE_DIR}/hd80M.img
    FIXTURES_REQUIRED FsImages
    RESOURCE_LOCK qemu_fs
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    LABELS fs syscall
)

knix_add_system_test(fs.syscall.metadata
    QEMU
    COMMAND
        ${Python3_EXECUTABLE}
        ${FS_QEMU_SCRIPT}
        test
        --scenario runner_fs_metadata
        --build-dir ${CMAKE_BINARY_DIR}
        --qemu-binary ${QEMU_SYSTEM_I386}
        --artifacts-dir ${FS_TEST_ARTIFACT_ROOT}/fs.syscall.metadata
        --os-image ${FS_SUITE_IMAGE_DIR}/hd64M.img
        --data-image ${FS_SUITE_IMAGE_DIR}/hd80M.img
    FIXTURES_REQUIRED FsImages
    RESOURCE_LOCK qemu_fs
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    LABELS fs syscall
)

knix_add_system_test(fs.syscall.error_paths
    QEMU
    COMMAND
        ${Python3_EXECUTABLE}
        ${FS_QEMU_SCRIPT}
        test
        --scenario runner_fs_error_paths
        --build-dir ${CMAKE_BINARY_DIR}
        --qemu-binary ${QEMU_SYSTEM_I386}
        --artifacts-dir ${FS_TEST_ARTIFACT_ROOT}/fs.syscall.error_paths
        --os-image ${FS_SUITE_IMAGE_DIR}/hd64M.img
        --data-image ${FS_SUITE_IMAGE_DIR}/hd80M.img
    FIXTURES_REQUIRED FsImages
    RESOURCE_LOCK qemu_fs
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    LABELS fs syscall
)

knix_add_system_test(fs.persistence.file.write
    QEMU
    COMMAND
        ${Python3_EXECUTABLE}
        ${FS_QEMU_SCRIPT}
        test
        --scenario runner_fs_persistence_write
        --build-dir ${CMAKE_BINARY_DIR}
        --qemu-binary ${QEMU_SYSTEM_I386}
        --artifacts-dir ${FS_TEST_ARTIFACT_ROOT}/fs.persistence.file.write
        --os-image ${FS_PERSIST_FILE_IMAGE_DIR}/hd64M.img
        --data-image ${FS_PERSIST_FILE_IMAGE_DIR}/hd80M.img
        --no-snapshot
    FIXTURES_REQUIRED FsImages
    FIXTURES_SETUP FsPersistFileWritten
    RESOURCE_LOCK qemu_fs
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    LABELS fs syscall persistence
)

knix_add_system_test(fs.persistence.file.verify
    QEMU
    COMMAND
        ${Python3_EXECUTABLE}
        ${FS_REQUIRE_SUCCESS_SCRIPT}
        --label fs.persistence.file.verify
        --required-marker ${FS_TEST_ARTIFACT_ROOT}/fs.persistence.file.write/scenario.pass
        --
        ${Python3_EXECUTABLE}
        ${FS_QEMU_SCRIPT}
        test
        --scenario runner_fs_persistence_verify
        --build-dir ${CMAKE_BINARY_DIR}
        --qemu-binary ${QEMU_SYSTEM_I386}
        --artifacts-dir ${FS_TEST_ARTIFACT_ROOT}/fs.persistence.file.verify
        --os-image ${FS_PERSIST_FILE_IMAGE_DIR}/hd64M.img
        --data-image ${FS_PERSIST_FILE_IMAGE_DIR}/hd80M.img
        --no-snapshot
    FIXTURES_REQUIRED "FsImages;FsPersistFileWritten"
    RESOURCE_LOCK qemu_fs
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    LABELS fs syscall persistence
    DEPENDS fs.persistence.file.write
    SKIP_RETURN_CODE 125
)

knix_add_system_test(fs.persistence.directory.write
    QEMU
    COMMAND
        ${Python3_EXECUTABLE}
        ${FS_QEMU_SCRIPT}
        test
        --scenario runner_fs_persistence_dir_write
        --build-dir ${CMAKE_BINARY_DIR}
        --qemu-binary ${QEMU_SYSTEM_I386}
        --artifacts-dir ${FS_TEST_ARTIFACT_ROOT}/fs.persistence.directory.write
        --os-image ${FS_PERSIST_DIR_IMAGE_DIR}/hd64M.img
        --data-image ${FS_PERSIST_DIR_IMAGE_DIR}/hd80M.img
        --no-snapshot
    FIXTURES_REQUIRED FsImages
    FIXTURES_SETUP FsPersistDirectoryWritten
    RESOURCE_LOCK qemu_fs
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    LABELS fs syscall persistence
)

knix_add_system_test(fs.persistence.directory.verify
    QEMU
    COMMAND
        ${Python3_EXECUTABLE}
        ${FS_REQUIRE_SUCCESS_SCRIPT}
        --label fs.persistence.directory.verify
        --required-marker ${FS_TEST_ARTIFACT_ROOT}/fs.persistence.directory.write/scenario.pass
        --
        ${Python3_EXECUTABLE}
        ${FS_QEMU_SCRIPT}
        test
        --scenario runner_fs_persistence_dir_verify
        --build-dir ${CMAKE_BINARY_DIR}
        --qemu-binary ${QEMU_SYSTEM_I386}
        --artifacts-dir ${FS_TEST_ARTIFACT_ROOT}/fs.persistence.directory.verify
        --os-image ${FS_PERSIST_DIR_IMAGE_DIR}/hd64M.img
        --data-image ${FS_PERSIST_DIR_IMAGE_DIR}/hd80M.img
        --no-snapshot
    FIXTURES_REQUIRED "FsImages;FsPersistDirectoryWritten"
    RESOURCE_LOCK qemu_fs
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    LABELS fs syscall persistence
    DEPENDS fs.persistence.directory.write
    SKIP_RETURN_CODE 125
)

knix_add_system_test(fs.regression.mkdir_rootqq_pf
    QEMU
    COMMAND
        ${Python3_EXECUTABLE}
        ${FS_QEMU_SCRIPT}
        test
        --scenario shell_fs_regression_mkdir_rootqq_pf
        --build-dir ${CMAKE_BINARY_DIR}
        --qemu-binary ${QEMU_SYSTEM_I386}
        --artifacts-dir ${FS_TEST_ARTIFACT_ROOT}/fs.regression.mkdir_rootqq_pf
        --os-image ${FS_SUITE_IMAGE_DIR}/hd64M.img
        --data-image ${FS_SUITE_IMAGE_DIR}/hd80M.img
    FIXTURES_REQUIRED FsImages
    RESOURCE_LOCK qemu_fs
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    LABELS fs shell regression
)
endif()
