set(target entt)
project(${target})

set(CMAKE_CXX_STANDARD 17)

set(srcs entt.cpp)

add_library(${target} STATIC ${srcs})

target_compile_definitions(${target} PUBLIC ENTT_NOEXCEPTION)

target_include_directories(${target}
		PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}

        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)