args = []
if meson.get_compiler('cpp').get_id() == 'gcc'
    args += [ '-Wno-switch' ]
endif

target_lib = static_library('tinyddsloader', 'tinyddsloader.cpp', cpp_args: args)
tinyddsloader_dep = declare_dependency(include_directories: '.', link_with: target_lib)
