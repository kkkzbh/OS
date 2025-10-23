

add_library(u_print)

target_sources(u_print
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        lib/print.cpp
)

target_link_libraries(u_print PRIVATE
        user
)

add_library(stdc++)

target_sources(stdc++
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        stdcxx.cpp
)

target_link_libraries(stdc++ PRIVATE
        u_print
        INTERFACE
        new
)

add_executable(program_no_arg
        program_no_arg.cpp
)

target_sources(program_no_arg
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
)

target_link_options(program_no_arg PRIVATE
        -e main
)

target_link_libraries(program_no_arg
        PRIVATE
        stdc++
)

# 将 program_no_arg 写入磁盘镜像到扇区 1000，预留 100 个扇区 (50KB)
add_disk_target(write_program_no_arg
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/program_no_arg
    1000
    100
)