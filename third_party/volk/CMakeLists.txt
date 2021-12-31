set(target volk)
project(${target})
add_library(${target} volk.c)

target_link_libraries(${target} dl)

target_include_directories(${target}
	PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}

	INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)