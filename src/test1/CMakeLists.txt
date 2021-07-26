include_directories(${PROJECT_SOURCE_DIR}/src/unicorn/include)
#include_directories(${PROJECT_SOURCE_DIR}/utils/unittest/qemu/include)
#link_directories(${PROJECT_SOURCE_DIR}/utils/unittest/unicorn/lib)

file(GLOB srclist *.c *.h)
add_executable(test1 ${srclist})
target_link_libraries(test1 mcore unicorn Shlwapi)

set_target_properties(test1 PROPERTIES
    VS_DEBUGGER_COMMAND_ARGUMENTS "${PROJECT_SOURCE_DIR}")