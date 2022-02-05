set(target sandbox)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${target})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/${target})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/${target})

project(${target} CXX)

set(SHADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/main.vert.glsl
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/phong.frag.glsl
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/color.frag.glsl
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.vert.glsl
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.frag.glsl
)

set(COMPILED_SHADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/main.vert.glsl.spv
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/phong.frag.glsl.spv
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/color.frag.glsl.spv
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.vert.glsl.spv
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.frag.glsl.spv
)

compile_shaders(${SHADERS})

add_executable(${target} main.cpp ${SHADERS} ${COMPILED_SHADERS})
target_link_libraries(${target} fluent-engine -fsanitize=address)
target_compile_options(${target} PRIVATE -fsanitize=address)