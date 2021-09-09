cmake_minimum_required(VERSION 3.16)

project(EZ_POOL
	VERSION 0.3.1
	LANGUAGES CXX
)

option(EZ_POOL_BUILD_TESTS "Build the test executables" ON)
set(EZ_POOL_CONFIG_DIR "share/ez-pool" CACHE STRING "The relative directory to install package config files.")

include(FetchContent)

FetchContent_Declare(
	ez-cmake
	GIT_REPOSITORY "https://github.com/errata-c/ez-cmake.git"
	GIT_TAG "main"
)
FetchContent_MakeAvailable(ez-cmake)


find_path(PHMAP_HEADERS "parallel_hashmap/phmap.h")

add_library(ez-pool INTERFACE)
target_compile_features(ez-pool INTERFACE cxx_std_17)
target_include_directories(ez-pool INTERFACE 
	"$<BUILD_INTERFACE:${PHMAP_HEADERS}>" 
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>" 
	"$<INSTALL_INTERFACE:include>"
)
target_compile_options(ez-pool INTERFACE "$<BUILD_INTERFACE:$<$<CXX_COMPILER_ID:MSVC>:/permissive->>")
set_target_properties(ez-pool PROPERTIES EXPORT_NAME "pool")
add_library(ez::pool ALIAS ez-pool)


if(EZ_POOL_BUILD_TESTS)
	add_subdirectory("tests")
endif()

install(DIRECTORY "include/" DESTINATION "include")

install(TARGETS "ez-pool" EXPORT "ez-pool-targets")

install_package(
	NAME "ez-pool"
	NAMESPACE "ez::"
	EXPORT "ez-pool-targets"
	VERSION "${EZ_POOL_VERSION}"
	DESTINATION "${EZ_POOL_CONFIG_DIR}"
	COMPATIBILITY SameMajorVersion
	ARCH_INDEPENDENT
	
	POSTCONFIG "cmake/postconfig.cmake"
)


