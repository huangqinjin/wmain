cmake_minimum_required(VERSION 3.3)
set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "Enable verbose output from builds." FORCE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_subdirectory(words)
add_subdirectory(test)
