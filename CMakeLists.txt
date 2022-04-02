cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0054 NEW) # ENABLE CMP0054: Only interpret if() arguments as variables or keywords when unquoted.
cmake_policy(SET CMP0042 NEW) # ENABLE CMP0042: MACOSX_RPATH is enabled by default.
cmake_policy(SET CMP0063 NEW) # ENABLE CMP0063: Honor visibility properties for all target types.
cmake_policy(SET CMP0077 NEW) # ENABLE CMP0077: option() honors normal variables

set(target fluent-engine)
project(${target} CXX)

set(CMAKE_CXX_STANDARD 17)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(FLUENT_DEBUG_BUILD ON)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set(FLUENT_DEBUG_BUILD OFF)
endif()

set(RENDERER_BACKEND_VULKAN False)
set(RENDERER_BACKEND_D3D12 False)

set(RENDERER_BACKEND "Vulkan" CACHE STRING "D3D12")

if ("${RENDERER_BACKEND}" STREQUAL "Vulkan")
    set(RENDERER_BACKEND_VULKAN True)
elseif("${RENDERER_BACKEND}" STREQUAL "D3D12")
    set(RENDERER_BACKEND_D3D12 True)
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/compile_shaders.cmake)
set(FLUENT_BUILD_TESTS ON)
set(BUILD_SHARED_LIBS OFF)

if (RENDERER_BACKEND_VULKAN)
    add_subdirectory(third_party/volk)
    add_subdirectory(third_party/vk_mem_alloc)
    set(include_dirs
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/vk_enum_string_helper)
    set(libs volk vma_allocator)
elseif (RENDERER_BACKEND_D3D12)
    add_subdirectory(third_party/D3D12MemoryAllocator)
    set(include_dirs ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dxc/inc)
    set(lib_dirs ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dxc/lib)
    set(libs d3d12 dxgi dxcompiler D3D12MemoryAllocator)
endif()

add_subdirectory(third_party/spdlog)
add_subdirectory(third_party/tiny_image_format)
add_subdirectory(third_party/spirv_reflect)
add_subdirectory(third_party/tinyddsloader)
add_subdirectory(third_party/SDL)
add_subdirectory(third_party/stb_image)
add_subdirectory(third_party/glm)
add_subdirectory(third_party/imgui)
add_subdirectory(third_party/entt)

set(libs
    spdlog
    SDL2::SDL2-static
    tiny_image_format
    spirv_reflect
    tinyddsloader
    stb_image
    glm
    imgui
    entt
    ${libs})

set(src_dir ${CMAKE_CURRENT_SOURCE_DIR}/sources)

set(core
    ${src_dir}/core/log.hpp
    ${src_dir}/core/log.cpp
    ${src_dir}/core/window.hpp
    ${src_dir}/core/window.cpp
    ${src_dir}/core/input.hpp
    ${src_dir}/core/input.cpp
    ${src_dir}/core/application.hpp
    ${src_dir}/core/application.cpp
    ${src_dir}/core/camera.hpp
    ${src_dir}/core/camera.cpp
    )

set(renderer
    ${src_dir}/renderer/shader_reflection.hpp
    ${src_dir}/renderer/shader_reflection.cpp
    ${src_dir}/renderer/renderer_backend.hpp
    ${src_dir}/renderer/vulkan_backend.cpp
    ${src_dir}/renderer/d3d12_backend.cpp
    ${src_dir}/renderer/resource_loader.hpp
    ${src_dir}/renderer/resource_loader.cpp)

set(util
    ${src_dir}/utils/file_loader.hpp
    ${src_dir}/utils/file_loader.cpp
    ${src_dir}/utils/image_loader.hpp
    ${src_dir}/utils/image_loader.cpp
    ${src_dir}/utils/file_system.hpp
    ${src_dir}/utils/file_system.cpp)

set(math
    ${src_dir}/math/math.hpp
    ${src_dir}/math/math.cpp)

set(sources
    ${core}
    ${renderer}
    ${resource_manager}
    ${util}
    ${math}
    ${scene})

set(include_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/sources
    ${include_dirs})

add_library(${target} ${sources})

target_include_directories(${target}
    PUBLIC
    ${include_dirs}

    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)

target_link_directories(${target} PUBLIC ${lib_dirs})
target_link_libraries(${target} PUBLIC ${libs})

if (RENDERER_BACKEND_VULKAN)
    set(compile_definitions VULKAN_BACKEND)
elseif(RENDERER_BACKEND_D3D12)
    set(compile_definitions D3D12_BACKEND)
endif()

if (FLUENT_DEBUG_BUILD)
    set(compile_definitions FLUENT_DEBUG ${compile_definitions})
    if (UNIX)
        set(compile_options -g)
    endif()
else()
    if (UNIX)
        target_compile_options(${target} PUBLIC -O3)
    endif()
endif()

target_compile_definitions(${target} PUBLIC ${compile_definitions})
target_compile_options(${target} PUBLIC ${compile_options})

if (FLUENT_BUILD_TESTS)
    add_subdirectory(examples)
endif()
