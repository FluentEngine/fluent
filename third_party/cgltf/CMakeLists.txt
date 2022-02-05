set(target cgltf)
project(${target})
add_library(${target} cgltf.cpp)

target_include_directories(${target}
	PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}

	INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)