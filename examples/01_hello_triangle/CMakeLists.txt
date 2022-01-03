set(target 01_hello_triangle)
project(${target} CXX)
add_executable(${target} main.cpp)
target_link_libraries(${target} fluent-engine)