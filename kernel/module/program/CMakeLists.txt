

add_library(stdc++)

target_sources(stdc++
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        stdcxx.cpp
        lib/io.cpp
        lib/os.cpp
        lib/container.cpp
)

target_link_libraries(stdc++ PRIVATE
        syscall
        std_array
        std_array_format
        std_string_format
        user_std_vector
        user_std_vector_format
        format
        INTERFACE
        new
)

add_executable(program_no_arg
        program_no_arg.cpp
)

target_link_options(program_no_arg PRIVATE
        -e main
)

target_link_libraries(program_no_arg
        PRIVATE
        stdc++
)

add_executable(program_arg 
        ${CMAKE_SOURCE_DIR}/kernel/crt/start.asm
        program_arg.cpp
)

target_link_options(program_arg PRIVATE
        -e _start
)

target_link_libraries(program_arg
        PRIVATE
        stdc++
)

# 将 program_no_arg 写入磁盘镜像到扇区 1000，预留 100 个扇区 (50KB)
add_disk_target(write_program_no_arg
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/program_no_arg
        1000
        100
)
add_dependencies(write_program_no_arg program_no_arg)

# 将 program_arg 写入磁盘镜像到扇区 1100，预留 200 个扇区 (100KB)
add_disk_target(write_program_arg
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/program_arg
        1100
        200
)
add_dependencies(write_program_arg program_arg)