project('fluent-engine', [ 'c' ], default_options : [ 'default_library=static', 'c_std=c11' ] )

ft_c_args = []
ft_objc_args = []

include_dirs = []

c = meson.get_compiler('c')

fluent_debug_build = get_option('buildtype') == 'debug'

if host_machine.system() == 'linux' or host_machine.system() == 'darwin'
    if fluent_debug_build
    	ft_c_args += [ '-g', '-DFLUENT_DEBUG' ]
	else
		ft_c_args += [ '-O3' ]
	endif
endif

if c.get_id() == 'msvc'
    ft_c_args += [ '-DNOMINMAX' ]
    ft_cpp_args += [ '-DNOMINMAX' ]
endif

# renderer

vulkan_include_directory = ''

vulkan_dep = dependency('vulkan', required: true)
renderer_backend_vulkan = vulkan_dep.found()

d3d12_dep = c.find_library('d3d12', required: get_option('fluent_d3d12_backend'))
renderer_backend_d3d12 = d3d12_dep.found()

mtl_dep = dependency('appleframeworks', modules : 'metal', required: get_option('fluent_metal_backend'))
renderer_backend_metal = mtl_dep.found()

if not c.check_header(get_option('vulkan_include_directory')/'vulkan/vulkan.h', dependencies: vulkan_dep, required: false)
    error('Set proper vulkan_include_directory option')
endif

if renderer_backend_vulkan
	ft_c_args += [ '-DVULKAN_BACKEND' ]
endif    

if renderer_backend_d3d12
    ft_c_args += [ '-DD3D12_BACKEND' ]
endif

if renderer_backend_metal
    add_languages( [ 'objc' ])
    ft_c_args += [ '-DMETAL_BACKEND' ]
    ft_objc_args += [ '-fobjc-arc', '-fobjc-weak', '-fmodules' ]
endif

subdir('sources/log')
subdir('sources/os')
subdir('sources/renderer')
