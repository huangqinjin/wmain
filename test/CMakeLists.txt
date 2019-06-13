cmake_minimum_required(VERSION 3.3)

project(execution_charset)
add_executable(${PROJECT_NAME} ${PROJECT_NAME}.c)
target_compile_definitions(${PROJECT_NAME} PRIVATE _UNICODE _CRT_SECURE_NO_WARNINGS)
target_compile_options(${PROJECT_NAME} PRIVATE /W4 /FC /source-charset:.1252)
