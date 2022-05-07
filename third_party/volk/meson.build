inc = include_directories('.', vulkan_include_directory, is_system : true)
target_lib = library('volk', 'volk.c', include_directories: inc)
volk_dep = declare_dependency(
    include_directories: inc,
	link_with: target_lib)
