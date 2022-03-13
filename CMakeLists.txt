cmake_minimum_required(VERSION 3.14)
project(wmain)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_library(${PROJECT_NAME} STATIC ${PROJECT_NAME}.c)
if(MINGW) # MinGW clang/gcc
    target_link_options(${PROJECT_NAME} INTERFACE -municode)
else() # Windows clang/clang-cl/cl
    # https://docs.microsoft.com/en-us/cpp/build/reference/wholearchive-include-all-library-object-files
    # By default, the linker includes object files from a specified static library in the linked output only if they
    # export symbols referenced by other object files in the executable. The /WHOLEARCHIVE option makes the linker treat
    # all object files archived in a static library as if they were specified individually on the linker command line.
    target_link_options(${PROJECT_NAME} INTERFACE LINKER:/WHOLEARCHIVE:$<TARGET_FILE:${PROJECT_NAME}>)
endif()

add_library(utf8 INTERFACE)
target_sources(utf8 INTERFACE wmain.manifest)

add_subdirectory(words)
add_subdirectory(test)
