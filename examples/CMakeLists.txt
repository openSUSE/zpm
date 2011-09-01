
include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(what_provides what_provides.c)
target_link_libraries(what_provides zpm)

