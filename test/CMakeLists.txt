cmake_minimum_required(VERSION 3.14)

project(execution_charset)
add_executable(${PROJECT_NAME} ${PROJECT_NAME}.c)
target_compile_definitions(${PROJECT_NAME} PRIVATE _UNICODE _CRT_SECURE_NO_WARNINGS)
target_compile_options(${PROJECT_NAME} PRIVATE /W4 /FC /source-charset:.1252)

project(naive_echo)
add_executable(${PROJECT_NAME} echo.c)
target_compile_definitions(${PROJECT_NAME} PRIVATE CP=cp)

project(utf8_echo)
add_executable(${PROJECT_NAME} echo.c)
target_link_libraries(${PROJECT_NAME} PRIVATE wmain)

project(utf8_acp)
add_executable(${PROJECT_NAME} echo.c)
target_link_libraries(${PROJECT_NAME} PRIVATE utf8)

project(direct_io)
add_executable(${PROJECT_NAME} ${PROJECT_NAME}.c)

project(wide_io)
add_executable(${PROJECT_NAME} ${PROJECT_NAME}.cpp)

project(utf8_io)
add_executable(${PROJECT_NAME} ${PROJECT_NAME}.cpp)
target_compile_options(${PROJECT_NAME} PRIVATE /W4 /wd4267 /utf-8 /std:c++17)

project(win32_gui)
add_library(${PROJECT_NAME} OBJECT ${PROJECT_NAME}.cpp)
target_compile_options(${PROJECT_NAME} PRIVATE /W4 /wd4458 /std:c++17)
target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE)

project(ansi_gui)
add_executable(${PROJECT_NAME} WIN32)
target_link_libraries(${PROJECT_NAME} PRIVATE win32_gui)

project(utf8_gui)
add_executable(${PROJECT_NAME} WIN32)
target_link_libraries(${PROJECT_NAME} PRIVATE win32_gui utf8)
