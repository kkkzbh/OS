

add_library(tss)

target_sources(tss
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
        tss.cpp
)

target_link_libraries(tss
        PRIVATE
        utility
        sync
        kernel_utility
        stdio
)
