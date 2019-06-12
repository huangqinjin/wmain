cmake_minimum_required(VERSION 3.3)
project(words)

add_executable(${PROJECT_NAME} main.c)

target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
