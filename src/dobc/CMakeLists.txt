file(GLOB srclist *.cc *.c *.hh *.h)
#add_executable(sleighexample ${srclist})
add_executable(dobc ${srclist})

#target_compile_definitions(sleighexample PUBLIC SLEIGHEXAMPLE)
target_compile_definitions(dobc PUBLIC DOBC)

#target_link_libraries(sleighexample mcore Shlwapi)
target_link_libraries(dobc sleigh_lib mcore Shlwapi)
