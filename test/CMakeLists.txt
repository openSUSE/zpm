enable_testing()

# make check framework headers available
# check.h is generated in build dir
include_directories(${CMAKE_SOURCE_DIR}/src)

macro(add_tests)
  foreach( loop_var ${ARGV} )
    #SET_SOURCE_FILES_PROPERTIES( ${loop_var}_test.c "" )
    add_executable( ${loop_var}_test ${loop_var}_test.cc )
    target_link_libraries( ${loop_var}_test ${CHECK_LIBRARIES} zpm)
    #MESSAGE( INFO "Adding test ${loop_var}_test ${CMAKE_CURRENT_BINARY_DIR}/${loop_var}_test")
    add_test( "${loop_var}_test" "${CMAKE_CURRENT_BINARY_DIR}/${loop_var}_test" )
  endforeach( loop_var )
endmacro(add_tests)

add_tests(zpm_checksum)

