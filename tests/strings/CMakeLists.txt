cmake_minimum_required(VERSION 3.15)

add_executable(string_test "strings.cpp")
target_link_libraries(string_test PRIVATE test_framework)