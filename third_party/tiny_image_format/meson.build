args = []

if meson.get_compiler('cpp').get_id() == 'clang'
    args += [ '-Wno-switch-enum' ]
endif

target_lib = static_library('tiny_image_format', 'tinyimageformat.cpp', cpp_args: args)
tinyimageformat_dep = declare_dependency(include_directories: '.', link_with: target_lib)
