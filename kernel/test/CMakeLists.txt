

add_library(test_os)

target_sources(test_os
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        filesystem.cpp
)

target_link_libraries(test_os
        kernel_module
)