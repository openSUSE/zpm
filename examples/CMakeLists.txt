
include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(what_provides what_provides.c)
target_link_libraries(what_provides zpm)
target_link_libraries(what_provides solvext)
target_link_libraries(what_provides solv)

add_executable(search search.c)
target_link_libraries(search zpm)
#target_link_libraries(what_provides solvext)
#target_link_libraries(what_provides solv)
