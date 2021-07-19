cmake_minimum_required(VERSION 3.15)

project(EZ_POOL_TESTS
VERSION 0.1)

find_package(fmt CONFIG REQUIRED)

# We use the Catch2 library for testing
find_package(Catch2 CONFIG REQUIRED)


add_executable(ez-pool-tests 
	"main.cpp"
	"basic/basic.cpp"
	"basic/block.cpp"
	"strings/strings.cpp"
)
target_link_libraries(ez-pool-tests PRIVATE 
	ez::pool
	fmt::fmt
	Catch2::Catch2
)