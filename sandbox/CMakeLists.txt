set(target sandbox)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${target})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/${target})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/${target})

project(${target} CXX)

file(GLOB SHADERS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.glsl)

foreach(source IN LISTS SHADERS)
  list(APPEND COMPILED_SHADERS ${source}.spv)
endforeach()

compile_shaders(${SHADERS})

add_executable(${target} main.cpp ${SHADERS} ${COMPILED_SHADERS})
target_link_libraries(${target} fluent-engine -fsanitize=address)
target_compile_options(${target} PRIVATE -fsanitize=address)