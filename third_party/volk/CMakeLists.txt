set(target volk)
project(${target})
add_library(${target} volk.c)

if (UNIX AND NOT APPLE)
	target_link_libraries(${target} dl)
endif()

target_include_directories(${target}
	PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}

	INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)