inc = include_directories('.', '../volk', vulkan_include_directory, is_system : true)
args = []

if meson.get_compiler('cpp').get_id() == 'clang'
	args += '-Wno-everything'
endif

if meson.get_compiler('cpp').get_id() == 'gcc'
	args += '-w'
endif

if meson.get_compiler('cpp').get_id() != 'msvc'
    args += '-Wno-nullability-completeness'
endif

target_lib = static_library('vma_allocator', 'vk_mem_alloc.cpp', include_directories : inc, cpp_args: args)
vma_allocator_dep = declare_dependency(
    include_directories : inc,
	link_with : target_lib)
