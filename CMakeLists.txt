cmake_minimum_required(VERSION 3.12)
project(dobc)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)


add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
add_compile_definitions(_WINSOCK_DEPRECATED_NO_WARNINGS)
#add_compile_definitions(UNICODE)
#add_compile_definitions(_UNICODE)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(${PROJECT_SOURCE_DIR}/src/)
include_directories(${PROJECT_SOURCE_DIR}/src/sleigh)

add_subdirectory(${PROJECT_SOURCE_DIR}/src/mcore)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/sleigh)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/dobc)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/test1)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/unicorn)