cmake_minimum_required(VERSION 2.6)
project(zpm)
find_package(Zypp REQUIRED)

add_subdirectory(src)
add_subdirectory(examples)