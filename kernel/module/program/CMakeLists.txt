

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
        program_arg.cpp
)

# 编译优化：减小体积 (仅C/C++，排除汇编)
target_compile_options(program_arg PRIVATE
        $<$<COMPILE_LANGUAGE:C,CXX>:-Os>
        $<$<COMPILE_LANGUAGE:C,CXX>:-fdata-sections>
        $<$<COMPILE_LANGUAGE:C,CXX>:-ffunction-sections>
)

target_link_options(program_arg PRIVATE
        -e main
        -Wl,--gc-sections       # 移除未使用的section
        -Wl,-s                  # 去除符号表
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

# 将 program_arg 写入磁盘镜像到扇区 1100，预留 100 个扇区 (50KB)
add_disk_target(write_program_arg
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/program_arg
        1100
        100
)
add_dependencies(write_program_arg program_arg)