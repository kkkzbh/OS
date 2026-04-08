list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(BootTests)
include(SystemTests)

add_subdirectory(boot)
add_subdirectory(system)
