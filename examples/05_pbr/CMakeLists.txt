set(target 05_pbr)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/${target})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/examples/${target})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/examples/${target})

project(${target} CXX)

set(SHADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../shaders/${target}/brdf_integration.comp.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/../shaders/${target}/irradiance.comp.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/../shaders/${target}/pano_to_cube.comp.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/../shaders/${target}/specular.comp.glsl
    
    ${CMAKE_CURRENT_SOURCE_DIR}/../shaders/${target}/skybox.vert.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/../shaders/${target}/skybox.frag.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/../shaders/${target}/pbr.vert.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/../shaders/${target}/pbr.frag.glsl
)

set(COMPILED_SHADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../shaders/${target}/brdf_integration.comp.glsl.spv
    ${CMAKE_CURRENT_SOURCE_DIR}/../shaders/${target}/irradiance.comp.glsl.spv
    ${CMAKE_CURRENT_SOURCE_DIR}/../shaders/${target}/pano_to_cube.comp.glsl.spv
    ${CMAKE_CURRENT_SOURCE_DIR}/../shaders/${target}/specular.comp.glsl.spv

    ${CMAKE_CURRENT_SOURCE_DIR}/../shaders/${target}/skybox.vert.glsl.spv
    ${CMAKE_CURRENT_SOURCE_DIR}/../shaders/${target}/skybox.frag.glsl.spv
    ${CMAKE_CURRENT_SOURCE_DIR}/../shaders/${target}/pbr.vert.glsl.spv
    ${CMAKE_CURRENT_SOURCE_DIR}/../shaders/${target}/pbr.frag.glsl.spv
)

compile_shaders(${SHADERS})

add_executable(${target} main.cpp ${SHADERS} ${COMPILED_SHADERS})
target_link_libraries(${target} fluent-engine)