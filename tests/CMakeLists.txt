cmake_minimum_required(VERSION 3.15)

project(EZ_POOL_TESTS
VERSION 0.1)

find_package(fmt CONFIG REQUIRED)

add_library(test_framework INTERFACE)
target_link_libraries(test_framework INTERFACE ez-pool fmt::fmt)

add_subdirectory("basic")
add_subdirectory("strings")