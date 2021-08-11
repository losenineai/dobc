file(GLOB srclist *.cc *.c *.hh)
#add_executable(sleighexample ${srclist})
add_executable(sleigh ${srclist})
add_library(sleigh_lib  STATIC ${srclist})

#target_compile_definitions(sleighexample PUBLIC SLEIGHEXAMPLE)
target_compile_definitions(sleigh PUBLIC SLEIGH)

#target_link_libraries(sleigh )
#target_link_libraries(sleighexample mcore Shlwapi)
