cmake_minimum_required(VERSION 3.15)

add_executable(basic_test "basic.cpp")
target_link_libraries(basic_test PRIVATE test_framework)

add_executable(block_test "block.cpp")
target_link_libraries(block_test PRIVATE test_framework)