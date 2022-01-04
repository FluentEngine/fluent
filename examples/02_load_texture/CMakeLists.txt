set(target 02_load_texture)
project(${target} CXX)
add_executable(${target} main.cpp)
target_link_libraries(${target} fluent-engine)