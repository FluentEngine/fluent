args = []

if meson.get_compiler('cpp').get_id() == 'clang'
    args += [ '-Wno-switch-enum' ]
endif

target_lib = static_library('tiny_image_format', 'tinyimageformat.c', include_directories: include_directories('.', is_system : true), cpp_args: args)
tinyimageformat_dep = declare_dependency(include_directories: include_directories('.', is_system : true), link_with: target_lib)
