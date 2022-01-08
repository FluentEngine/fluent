set(target 04_model_loading)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/${target})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/examples/${target})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/examples/${target})

project(${target} CXX)

set(SHADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/../shaders/${target}/main.vert.glsl
  ${CMAKE_CURRENT_SOURCE_DIR}/../shaders/${target}/main.frag.glsl
)

set(COMPILED_SHADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/../shaders/${target}/main.vert.glsl.spv
  ${CMAKE_CURRENT_SOURCE_DIR}/../shaders/${target}/main.frag.glsl.spv
)

compile_shaders(${SHADERS})

add_executable(${target} main.cpp ${SHADERS} ${COMPILED_SHADERS})
target_link_libraries(${target} fluent-engine)