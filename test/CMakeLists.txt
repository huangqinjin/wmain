cmake_minimum_required(VERSION 3.3)

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
if(CMAKE_VERSION VERSION_LESS 3.13)
    # Manually specify the entry point for an .exe file.
    set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY LINK_FLAGS /ENTRY:wWinMainCRTStartup)
endif()
