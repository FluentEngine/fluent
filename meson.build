project('fluent-engine', ['c', 'cpp', 'objcpp' ], default_options : ['default_library=static', 'cpp_std=c++17'] )
target = 'fluent-engine'

fluent_debug_build = true
fluent_build_tests = true

renderer_backend_vulkan = true
renderer_backend_d3d12 = get_option('RENDERER_BACKEND_D3D12')

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
    if host_machine.system() == 'Linux' or host_machine.system() == 'MacOS'
        compile_args += ['-g']
    endif
else
    if host_machine.system() == 'Linux' or host_machine.system() == 'MacOS'
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
    include_dirs += [ include_directories('third_party/dxc/inc') ]
    lib_dirs += [ 'third_party/dxc/lib' ]
    libs += [ 'd3d12', 'dxgi', 'dxcompiler', 'D3D12MemoryAllocator' ]
endif

sdl2 = subproject('sdl2')
spdlog = subproject('spdlog')
glm = subproject('glm')
subdir('third_party/tiny_image_format')
subdir('third_party/tinyddsloader')
subdir('third_party/stb_image')
subdir('third_party/imgui')

deps += [ 
    dependency('sdl2'),
    dependency('spdlog'),
    dependency('glm') 
]

deps += [ 
    imgui_dep,
    tinyimageformat_dep,
    volk_dep,
    tinyddsloader_dep,
    stb_image_dep,
    sdl2.get_variable('sdl2_dep'),
    spdlog.get_variable('spdlog_dep'),
    glm.get_variable('glm_dep')
]

src_dir = 'sources'
core = [ 
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
]

renderer = [
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
]

util = [
    src_dir/'utils/file_loader.hpp', 
    src_dir/'utils/file_loader.cpp', 
    src_dir/'utils/image_loader.hpp', 
    src_dir/'utils/image_loader.cpp', 
    src_dir/'utils/file_system.hpp', 
    src_dir/'utils/file_system.cpp'
]

math = [
    src_dir/'math/math.hpp', 
    src_dir/'math/math.cpp'
]

sources = [
    core, 
    renderer, 
    util, 
    math
]

target_lib = library(target, sources, dependencies: deps, include_directories: 'sources', cpp_args: compile_args)
fluent_engine_dep = declare_dependency(include_directories: 'sources', link_with: target_lib, dependencies : deps)

glsllang = find_program('glslangValidator')
generate_spirv_args = [ '-e', 'main', '-V', '@INPUT@', '-o', '@OUTPUT@' ]

if run_command(glsllang, [ '--quiet', '--version' ], check : false).returncode() == 0
	generate_spirv_args += [ '--quiet' ]
endif

spirv_generator = generator(glsllang,
	output    : [ '@BASENAME@.bin' ],
	arguments : generate_spirv_args,
)

if fluent_build_tests
    subdir('examples')
endif
