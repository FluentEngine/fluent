set(target vma_allocator)
project(${target})

set(CMAKE_CXX_STANDARD 17)

set(srcs vk_mem_alloc.cpp)

add_library(${target} STATIC ${srcs})

if (UNIX)
    target_compile_options(${target} PUBLIC -Wno-nullability-completeness)
endif()

set(include_paths ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../volk/)

target_include_directories(${target}
        PUBLIC
        ${include_paths}

        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
