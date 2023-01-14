cmake_minimum_required(VERSION 3.21)

project(EZ_POOL
	VERSION 0.4.1
	LANGUAGES CXX
	DESCRIPTION "A simple memory and object pooling library"
)

include(FetchContent)
include(GNUInstallDirs)

FetchContent_Declare(
	ez-cmake
	GIT_REPOSITORY "https://github.com/errata-c/ez-cmake.git"
	GIT_TAG "main"
)
FetchContent_MakeAvailable(ez-cmake)

set(EZ_POOL_CONFIG_DIR "${CMAKE_INSTALL_DATADIR}/ez-pool" CACHE STRING "The relative directory to install package config files.")

add_library(ez-pool INTERFACE)
target_compile_features(ez-pool INTERFACE cxx_std_17)
target_include_directories(ez-pool INTERFACE 
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>" 
	"$<INSTALL_INTERFACE:include>"
)
target_compile_options(ez-pool INTERFACE "$<BUILD_INTERFACE:$<$<CXX_COMPILER_ID:MSVC>:/permissive->>")
set_target_properties(ez-pool PROPERTIES EXPORT_NAME "pool")
add_library(ez::pool ALIAS ez-pool)

if(PROJECT_IS_TOP_LEVEL)
	include(CTest)
	if(BUILD_TESTING)
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
	)
endif()