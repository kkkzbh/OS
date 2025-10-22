

add_library(libstdc++)

target_sources(libstdc++
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        libstdcxx.cpp
)

target_link_libraries(libstdc++ PRIVATE
        std
        std_algorithm
        std_array
        stdfmt
        filesystem
        user
        INTERFACE
        new
)

add_library(program
        program_no_arg.cpp
        libstdcxx.cpp
)

target_sources(program
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
)

target_link_libraries(shell
        PRIVATE
        libstdc++
)