sdl2 = subproject('sdl2')
glm = subproject('glm')
imgui = subproject('imgui')

deps = [
    sdl2.get_variable('sdl2_dep'),
    glm.get_variable('glm_dep'),
    imgui.get_variable('imgui_dep')
]

sources = files([
    '../fs/fs.h',
    '../fs/fs.c',

	'application.c', 
    'application.h',
    'input.c',
    'input.h',
    'key_codes.h',
    'mouse_codes.h',
    'window.c', 
    'window.h', 
])

ft_os_lib = library('ft_os', sources, 
    dependencies: deps, 
    include_directories: include_directories('../'),
    c_args : ft_c_args,
    objc_args: ft_objc_args)


ft_os_dep = declare_dependency(include_directories: include_directories('../'), 
	link_with: ft_os_lib,
	dependencies : [ deps, ft_log_dep ])
