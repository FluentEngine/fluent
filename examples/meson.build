examples = [ 
    '01_hello_triangle',
    '02_load_texture',
    '03_compute'
]

shaders = [
    'shaders/01_hello_triangle/main.vert.hlsl',
    'shaders/01_hello_triangle/main.frag.hlsl',
    'shaders/02_load_texture/main.vert.hlsl',
    'shaders/02_load_texture/main.frag.hlsl',
    'shaders/03_compute/main.comp.hlsl'
]

foreach shader : shaders
    r = run_command('glslangvalidator', '-e', 'main', '-V', meson.current_source_dir()/shader, '-o', meson.current_source_dir()/shader.replace('.hlsl', '.bin'), check: false)
    if r.returncode() != 0
        output = r.stdout().strip()
        errortxt = r.stderr().strip()
        message(output, errortxt)
    endif
endforeach

foreach example : examples
    srcs = files([
        example/'main.cpp',
        'common/sample.hpp'
    ])

    executable(example, srcs,
        dependencies	: fluent_engine_dep,
        cpp_args		: compile_args,
    )
endforeach
