include_directories(${PROJECT_SOURCE_DIR}/utils/unittest/unicorn/include)
include_directories(${PROJECT_SOURCE_DIR}/utils/unittest/qemu/include)
link_directories(${PROJECT_SOURCE_DIR}/utils/unittest/unicorn/lib)

file(GLOB srclist *.c)
add_executable(test1 ${srclist})
target_link_libraries(test1 unicorn_static)