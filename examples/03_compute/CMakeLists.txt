set(target 03_compute)
project(${target} CXX)
add_executable(${target} main.cpp)
target_link_libraries(${target} fluent-engine)