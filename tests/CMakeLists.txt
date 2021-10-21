cmake_minimum_required(VERSION 3.16)
project(EZ_POOL_TESTS)


find_package(fmt CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)


add_executable(ez_pool_tests 
	"main.cpp"
	"basic.cpp"
	"block.cpp"
)
target_link_libraries(ez_pool_tests PRIVATE 
	ez::pool
	fmt::fmt
	Catch2::Catch2
)