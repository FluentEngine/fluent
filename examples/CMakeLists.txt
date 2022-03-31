function(add_example target)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/${target})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/examples/${target})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/examples/${target})

    project(${target} CXX)

    compile_shaders(shaders "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${target}/")
    add_executable(${target}
        common/sample.hpp
        ${target}/main.cpp)

    target_link_libraries(${target} fluent-engine)
endfunction()

add_example(01_hello_triangle)
add_example(02_load_texture)
add_example(03_compute)
