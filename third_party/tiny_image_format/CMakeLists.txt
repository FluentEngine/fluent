set(target tiny_image_format)
project(${target})

set(CMAKE_CXX_STANDARD 17)

set(srcs tinyimageformat.cpp)

add_library(${target} STATIC ${srcs})

target_include_directories(${target}
		PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}

        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)