set(target volk)
project(${target})
add_library(${target} volk.c)

target_include_directories(${target}
	PUBLIC
    "./"

	INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)