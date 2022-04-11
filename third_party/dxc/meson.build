dxc_lib = 'dxcompiler'
dxc_lib_dir = 'lib'
dxc_inc_dir = 'inc'

dxc_dep = declare_dependency(
    link_args : ['-L' + dxc_lib_dir, '-l' + dxc_lib],
    include_directories : include_directories(dxc_inc_dir))
