subdir('third_party/tiny_image_format')
subdir('third_party/tinyddsloader')
subdir('third_party/stb')
subdir('third_party/hashmap.c')
imgui = subproject('imgui')

deps = [
	ft_log_dep,
    tinyimageformat_dep,
    tinyddsloader_dep,
    stb_image_dep,
    hashmap_dep,
    imgui.get_variable('imgui_dep')
]

if renderer_backend_vulkan
    vulkan_include_directory = get_option('vulkan_include_directory')
    include_dirs += include_directories(vulkan_include_directory, is_system : true)
endif

if renderer_backend_vulkan
    subdir('third_party/volk')
    subdir('third_party/vk_mem_alloc')
    subdir('third_party/spirv_reflect')

    deps += [
	    volk_dep,
        vma_allocator_dep,
        spirv_reflect_dep,
    ]
endif

if renderer_backend_d3d12
    subdir('third_party/D3D12MemoryAllocator')
    subdir('third_party/dxc')

    d3d12_lib = c.find_library('d3d12', required : true)
    dxgi_lib = c.find_library('dxgi', required : true)

    deps += [
        d3d12_allocator_dep,
        dxc_dep,
        d3d12_lib,
        dxgi_lib
    ]
endif

sources = files([
    'shader_reflection.h', 
    'renderer_enums.h',
    'renderer_backend.h',
    'renderer_backend_functions.h',
    'renderer_backend.c', 
    'vulkan/vulkan_backend.h', 
    'vulkan/vulkan_backend.c',
    'vulkan/vulkan_reflection.c',
    'd3d12/d3d12_backend.h', 
    'd3d12/d3d12_backend.c',
    'd3d12/d3d12_reflection.c',
    'resource_loader.h', 
    'resource_loader.c'
])

if renderer_backend_metal
    sources += files([
        'metal/metal_backend.h', 
        'metal/metal_backend.m',
        'metal/metal_reflection.m'
    ])
endif

ft_renderer_lib = library('ft_renderer', sources,
	dependencies		: deps,
	include_directories	: include_directories('../'),
	c_args				: ft_c_args,
	objc_args			: ft_objc_args)

ft_renderer_dep = declare_dependency(
	include_directories	: include_directories('../'), 
	link_with			: ft_renderer_lib,
	dependencies		: deps )
