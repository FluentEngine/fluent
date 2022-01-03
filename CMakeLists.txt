cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0054 NEW) # ENABLE CMP0054: Only interpret if() arguments as variables or keywords when unquoted.
cmake_policy(SET CMP0042 NEW) # ENABLE CMP0042: MACOSX_RPATH is enabled by default.
cmake_policy(SET CMP0063 NEW) # ENABLE CMP0063: Honor visibility properties for all target types.
cmake_policy(SET CMP0077 NEW) # ENABLE CMP0077: option() honors normal variables


add_subdirectory(third_party/spdlog)
add_subdirectory(third_party/volk)
add_subdirectory(third_party/SDL)
add_subdirectory(third_party/tiny_image_format)

set(CMAKE_CXX_STANDARD 17)

set(target fluent-engine)
project(${target} CXX)

set(src_dir ${CMAKE_CURRENT_SOURCE_DIR}/sources)

set(common_sources 
	${src_dir}/core/log.cpp
    ${src_dir}/core/window.cpp
	${src_dir}/core/application.cpp
	)

set(renderer_sources
	${src_dir}/renderer/renderer.cpp)

set(util_sources
	${src_dir}/utils/utils.cpp)

set(sources ${common_sources} ${renderer_sources} ${util_sources})
set(includes ${CMAKE_CURRENT_SOURCE_DIR}/sources)
set(libs spdlog volk SDL2 tiny_image_format)

add_library(${target} ${sources})

target_include_directories(${target}
	PUBLIC
    ${includes}

	INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)

target_link_libraries(${target} PUBLIC ${libs})

set(FLUENT_BUILD_TESTS ON)
set(FLUENT_DEBUG_BUILD ON)

if (FLUENT_DEBUG_BUILD)
	set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
	set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
	target_compile_definitions(${target} PUBLIC FLUENT_DEBUG)
	target_compile_options(${target} PUBLIC -g)
else()
	target_compile_options(${target} PUBLIC -O3)
endif()

if (FLUENT_BUILD_TESTS)
    add_subdirectory(examples)
endif()