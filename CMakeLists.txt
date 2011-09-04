cmake_minimum_required(VERSION 2.6)
project(zpm)

# TODO
# FIND libsolvg

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules )

find_package(Check REQUIRED)

add_subdirectory(src)
add_subdirectory(examples)
add_subdirectory(test)