cmake_minimum_required(VERSION 3.15)

project(EZ_POOL
VERSION 0.1
LANGUAGES CXX)

include(GNUInstallDirs)

find_package(ez-cmake CONFIG REQUIRED)

find_path(PHMAP_HEADERS "parallel_hashmap/phmap.h")

add_library(ez-pool INTERFACE)
target_compile_features(ez-pool INTERFACE cxx_std_17)
target_include_directories(ez-pool INTERFACE 
	"$<BUILD_INTERFACE:${PHMAP_HEADERS}>" 
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>" 
	"$<INSTALL_INTERFACE:include>"
)
target_compile_options(ez-pool INTERFACE "$<BUILD_INTERFACE:$<$<CXX_COMPILER_ID:MSVC>:/permissive->>")

add_subdirectory("tests")

set(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/ez-pool)

install(
	DIRECTORY "include/"
	DESTINATION "include"
)

install(TARGETS ez-pool
EXPORT ez-pool-targets
)

install_package(
NAME ez-pool
EXPORT ez-pool-targets
VERSION ${EZ_POOL_VERSION}
DESTINATION DEFAULT
COMPATIBILITY SameMajorVersion
ARCH_INDEPENDENT
POSTCONFIG "${CMAKE_CURRENT_SOURCE_DIR}/postconfig.cmake"
)


