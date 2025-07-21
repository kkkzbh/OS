cmake_minimum_required(VERSION 4.0.1)

project(os LANGUAGES C CXX ASM_NASM)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_C_STANDARD 23)

add_subdirectory(kernel)