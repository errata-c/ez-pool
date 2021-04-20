cmake_minimum_required(VERSION 3.15)

project(EZ_POOL
VERSION 0.2.1
LANGUAGES CXX)

include(GNUInstallDirs)
include("cmake/ez-cmake/install_package.cmake")

option(BUILD_TESTS "Build the test executables" ON)
option(SEPARATE_DIRECTORY "Install the package into its own directory" ON)
set(CONFIG_DIR "${CMAKE_INSTALL_DATAROOTDIR}/ez-pool" CACHE STRING "The relative directory to install package config files.")



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



if(BUILD_TESTS)
	add_subdirectory("tests")
endif()

if(SEPARATE_DIRECTORY)
	set(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/ez-pool)
endif()



install(DIRECTORY "include/" DESTINATION "include")

install(TARGETS "ez-pool" EXPORT "ez-pool-targets")

install_package(
	NAME "ez-pool"
	NAMESPACE "ez::"
	EXPORT "ez-pool-targets"
	VERSION "${EZ_POOL_VERSION}"
	DESTINATION "${CONFIG_DIR}"
	COMPATIBILITY SameMajorVersion
	ARCH_INDEPENDENT
	
	POSTCONFIG "cmake/postconfig.cmake"
)


