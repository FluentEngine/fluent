function(compile_shaders ${SHADERS})
    foreach(SHADER ${SHADERS})
        add_custom_command(OUTPUT ${SHADER}.spv
        COMMAND glslangValidator -V ${SHADER} -o ${SHADER}.spv
        DEPENDS ${SHADER}
        COMMENT "Rebuilding ${SHADER}.spv" )
        message(STATUS "Generating build commands for ${SHADER}.spv")
    endforeach()
endfunction()

cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0054 NEW) # ENABLE CMP0054: Only interpret if() arguments as variables or keywords when unquoted.
cmake_policy(SET CMP0042 NEW) # ENABLE CMP0042: MACOSX_RPATH is enabled by default.
cmake_policy(SET CMP0063 NEW) # ENABLE CMP0063: Honor visibility properties for all target types.
cmake_policy(SET CMP0077 NEW) # ENABLE CMP0077: option() honors normal variables

set(BUILD_SHARED_LIBS OFF)
set(ASSIMP_BUILD_TESTS OFF)
add_subdirectory(third_party/spdlog)
add_subdirectory(third_party/volk)
add_subdirectory(third_party/vk_mem_alloc)
add_subdirectory(third_party/SDL)
add_subdirectory(third_party/tiny_image_format)
add_subdirectory(third_party/spirv_reflect)
add_subdirectory(third_party/tinyddsloader)
add_subdirectory(third_party/stb_image)
add_subdirectory(third_party/glm)
add_subdirectory(third_party/tinygltf)
add_subdirectory(third_party/assimp)
add_subdirectory(third_party/imgui)
add_subdirectory(third_party/entt)

set(CMAKE_CXX_STANDARD 17)

set(target fluent-engine)
project(${target} CXX)

set(src_dir ${CMAKE_CURRENT_SOURCE_DIR}/sources)

set(core
	${src_dir}/core/log.cpp
    ${src_dir}/core/window.cpp
	${src_dir}/core/input.cpp
	${src_dir}/core/application.cpp
	)

set(renderer
	${src_dir}/renderer/shader_reflection.cpp
	${src_dir}/renderer/renderer.cpp)

set(resource_manager
	${src_dir}/resource_manager/resource_manager.cpp)
	
set(util
	${src_dir}/utils/utils.cpp
	${src_dir}/utils/model_loader.cpp)

set(math
	${src_dir}/math/math.cpp)

set(scene
	${src_dir}/scene/scene.cpp)

set(sources 
	${core} 
	${renderer}
	${resource_manager}
	${util} 
	${math}
	${scene})

set(includes ${CMAKE_CURRENT_SOURCE_DIR}/sources)

set(libs 
	spdlog 
	volk 
	vma_allocator 
	SDL2::SDL2-static 
	tiny_image_format
	spirv_reflect 
	tinyddsloader 
	stb_image 
	glm 
	tinygltf 
	assimp 
	imgui
	entt)

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
	set(compile_definitions FLUENT_DEBUG)
	if (MSVC)
		set(compile_options /DEBUG)
	elseif (UNIX)
		set(compile_options -g)
	endif()
else()
	if (MSVC)
	elseif (UNIX)
		target_compile_options(${target} PUBLIC -O3 -fsanitize=address)
	endif()
endif()

target_compile_definitions(${target} PUBLIC ${compile_definitions})
target_compile_options(${target} PUBLIC ${compile_options})

if (FLUENT_BUILD_TESTS)
    add_subdirectory(examples)
endif()

add_subdirectory(sandbox)