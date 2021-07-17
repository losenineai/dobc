cmake_minimum_required (VERSION 2.8.11)

project (test_runner)

set(topdir ${CMAKE_SOURCE_DIR}/..)

include_directories(${topdir}/utils/unittest/unicorn/include)
include_directories(${topdir}/utils/unittest/qemu/include)
link_directories(${topdir}/utils/unittest/unicorn/lib)

add_executable(test_runner test_runner.c)
target_link_libraries(test_runner unicorn_static)