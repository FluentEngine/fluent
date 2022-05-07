inc = include_directories('.', is_system : true)
target_lib = library('volk', 'volk.c', include_directories: inc)
volk_dep = declare_dependency(
    include_directories: inc,
	link_with: target_lib,
	dependencies : dependency('vulkan', required: false))
