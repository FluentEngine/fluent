project('fluent-engine', [ 'c', 'cpp' ], default_options : ['default_library=static', 'c_std=c17', 'cpp_std=c++17'] )
target = 'fluent-engine'

cc = meson.get_compiler('cpp')

if host_machine.system() == 'darwin'
    add_project_arguments(language : 'objcpp')
endif

fluent_debug_build = get_option('buildtype') == 'debug'
fluent_build_tests = get_option('fluent_build_tests')

dep = cc.find_library('vulkan-1', required: get_option('fluent_vulkan_backend'))
renderer_backend_vulkan = dep.found()
dep = cc.find_library('d3d12', required: get_option('fluent_d3d12_backend'))
renderer_backend_d3d12 = dep.found()

include_dirs = []
libs = []
deps = []
compile_args = []

if renderer_backend_vulkan
    compile_args = ['-DVULKAN_BACKEND', compile_args]
endif

if renderer_backend_d3d12
    compile_args += ['-DD3D12_BACKEND']
endif

if fluent_debug_build
    compile_args += ['-DFLUENT_DEBUG']
    if host_machine.system() == 'linux' or host_machine.system() == 'darwin'
        compile_args += ['-g']
    endif
else
    if host_machine.system() == 'linux' or host_machine.system() == 'darwin'
        compile_args += ['-O3']
    endif
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
subdir('third_party/tiny_image_format')
subdir('third_party/tinyddsloader')
subdir('third_party/stb_image')
subdir('third_party/imgui')

deps += [ 
    imgui_dep,
    tinyimageformat_dep,
    tinyddsloader_dep,
    stb_image_dep,
    sdl2.get_variable('sdl2_dep'),
    spdlog.get_variable('spdlog_dep'),
    glm.get_variable('glm_dep')
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
    src_dir/'renderer/shader_reflection.cpp', 
    src_dir/'renderer/renderer_backend.hpp', 
    src_dir/'renderer/renderer_backend.cpp', 
    src_dir/'renderer/vulkan/vulkan_backend.hpp', 
    src_dir/'renderer/vulkan/vulkan_backend.cpp', 
    src_dir/'renderer/d3d12/d3d12_backend.hpp', 
    src_dir/'renderer/d3d12/d3d12_backend.cpp', 
    src_dir/'renderer/resource_loader.hpp', 
    src_dir/'renderer/resource_loader.cpp'
])

fs = files([
    src_dir/'fs/file_system.hpp',
    src_dir/'fs/file_system.cpp'
])

math = [
    src_dir/'math/math.hpp', 
    src_dir/'math/math.cpp'
]

sources = [
    core, 
    renderer, 
    math,
    fs,
]

target_lib = library(target, sources, dependencies: deps, include_directories: 'sources', cpp_args: compile_args)
fluent_engine_dep = declare_dependency(include_directories: 'sources', link_with: target_lib, dependencies : deps)

if fluent_build_tests
    subdir('examples')
endif
