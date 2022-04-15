inc = include_directories('.', '../volk/', '../vulkan-headers/')
args = []

if meson.get_compiler('cpp').get_id() != 'msvc'
    args += '-Wno-nullability-completeness'
endif

target_lib = static_library('vma_allocator', 'vk_mem_alloc.cpp', include_directories : inc, cpp_args: args)
vma_allocator_dep = declare_dependency(include_directories: inc, link_with: target_lib)
