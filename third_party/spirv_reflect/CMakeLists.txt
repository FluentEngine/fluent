set(target spirv_reflect)
project(${target})

set(srcs spirv_reflect.c)

add_library(${target} STATIC ${srcs})

set(include_paths ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(${target}
        PUBLIC
        ${include_paths}

        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)