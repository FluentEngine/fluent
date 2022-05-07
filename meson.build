project('fluent-engine', [ 'c', 'cpp' ], default_options : ['default_library=static', 'c_std=c17', 'cpp_std=c++17'] )
target = 'fluent-engine'

ft_cpp_args = []
ft_objcpp_args = []
include_dirs = []
libs = []
deps = []

vulkan_include_directory = ''

c = meson.get_compiler('c')
cc = meson.get_compiler('cpp')

fluent_debug_build = get_option('buildtype') == 'debug'

vulkan_dep = dependency('vulkan', required: true)
renderer_backend_vulkan = vulkan_dep.found()
d3d12_dep = cc.find_library('d3d12', required: get_option('fluent_d3d12_backend'))
renderer_backend_d3d12 = d3d12_dep.found()
mtl_dep = dependency('appleframeworks', modules : 'metal', required: get_option('fluent_metal_backend'))
renderer_backend_metal = mtl_dep.found()

if not c.check_header(get_option('vulkan_include_directory')/'vulkan/vulkan.h', dependencies: vulkan_dep, required: false)
    error('Set proper vulkan_include_directory option')
endif

if renderer_backend_vulkan
    vulkan_include_directory = get_option('vulkan_include_directory')
    include_dirs += include_directories(vulkan_include_directory, is_system : true)
endif

if renderer_backend_vulkan
    ft_cpp_args += [ '-DVULKAN_BACKEND' ]
endif

if renderer_backend_d3d12
    ft_cpp_args += [ '-DD3D12_BACKEND' ]
endif

if renderer_backend_metal
    add_languages( [ 'objc', 'objcpp' ])
    ft_cpp_args += [ '-DMETAL_BACKEND' ]
    ft_objcpp_args += [ '-fobjc-arc', '-fobjc-weak', '-fmodules' ]
endif

if host_machine.system() == 'linux' or host_machine.system() == 'darwin'
    if fluent_debug_build
	    ft_cpp_args += [ '-g', '-DFLUENT_DEBUG' ]
	else
	    ft_cpp_args += [ '-O3' ]
	endif
endif

if cc.get_id() == 'msvc'
    ft_cpp_args += [ '-DNOMINMAX' ]
endif

if renderer_backend_vulkan
    subdir('third_party/volk')
    subdir('third_party/vk_mem_alloc')
    subdir('third_party/spirv_reflect')
    subdir('third_party/vk_enum_string_helper')

    deps += [
	    volk_dep,
        vma_allocator_dep,
        spirv_reflect_dep,
        vk_enum_string_helper_dep
    ]
endif

if renderer_backend_d3d12
    subdir('third_party/D3D12MemoryAllocator')
    subdir('third_party/dxc')

    d3d12_lib = cc.find_library('d3d12', required : true)
    dxgi_lib = cc.find_library('dxgi', required : true)

    deps += [
        d3d12_allocator_dep,
        dxc_dep,
        d3d12_lib,
        dxgi_lib
    ]
endif

sdl2 = subproject('sdl2')
spdlog = subproject('spdlog')
glm = subproject('glm')
imgui = subproject('imgui', default_options : 'vulkan_include_directory=' + vulkan_include_directory)
subdir('third_party/tiny_image_format')
subdir('third_party/tinyddsloader')
subdir('third_party/stb_image')

deps += [ 
    tinyimageformat_dep,
    tinyddsloader_dep,
    stb_image_dep,
    sdl2.get_variable('sdl2_dep'),
    spdlog.get_variable('spdlog_dep'),
    glm.get_variable('glm_dep'),
    imgui.get_variable('imgui_dep')
]

src_dir = 'sources'
core = files([
    src_dir/'core/log.hpp', 
    src_dir/'core/log.cpp', 
    src_dir/'core/window.hpp', 
    src_dir/'core/window.cpp', 
    src_dir/'core/input.hpp', 
    src_dir/'core/input.cpp', 
    src_dir/'core/application.hpp', 
    src_dir/'core/application.cpp', 
    src_dir/'core/camera.hpp', 
    src_dir/'core/camera.cpp'
])

renderer = files([
    src_dir/'renderer/shader_reflection.hpp', 
    src_dir/'renderer/renderer_enums.hpp',
    src_dir/'renderer/renderer_backend.hpp', 
    src_dir/'renderer/renderer_backend.cpp', 
    src_dir/'renderer/vulkan/vulkan_backend.hpp', 
    src_dir/'renderer/vulkan/vulkan_backend.cpp',
    src_dir/'renderer/vulkan/vulkan_reflection.cpp',
    src_dir/'renderer/d3d12/d3d12_backend.hpp', 
    src_dir/'renderer/d3d12/d3d12_backend.cpp',
    src_dir/'renderer/d3d12/d3d12_reflection.cpp',
    src_dir/'renderer/resource_loader.hpp', 
    src_dir/'renderer/resource_loader.cpp'
])

if renderer_backend_metal
    renderer += files([
        src_dir/'renderer/metal/metal_backend.hpp', 
        src_dir/'renderer/metal/metal_backend.mm',
        src_dir/'renderer/metal/metal_reflection.mm'
    ])
endif

fs = files([
    src_dir/'fs/fs.hpp',
    src_dir/'fs/fs.cpp'
])

math = files([
    src_dir/'math/math.hpp', 
    src_dir/'math/math.cpp'
])

sources = [
    core, 
    renderer, 
    math,
    fs,
]

target_lib = library(target, sources, 
    dependencies: deps, 
    include_directories: 'sources', 
    cpp_args: ft_cpp_args,
    objc_args: ft_objcpp_args,
    objcpp_args: ft_objcpp_args)

fluent_engine_dep = declare_dependency(include_directories: 'sources', link_with: target_lib, dependencies : deps)

