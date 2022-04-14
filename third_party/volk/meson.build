inc = include_directories('.', '../vulkan-headers/')
target_lib = library('volk', 'volk.c', include_directories: inc)
volk_dep = declare_dependency(include_directories: inc, link_with: target_lib)
