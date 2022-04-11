examples = [ 
    '01_hello_triangle',
    '02_load_texture',
    '03_compute'
]

vert_shaders = [
    'shaders/01_hello_triangle/main.vert.hlsl',
    'shaders/02_load_texture/main.vert.hlsl',
]

frag_shaders = [
    'shaders/01_hello_triangle/main.frag.hlsl',
    'shaders/02_load_texture/main.frag.hlsl',
]

comp_shaders = [
    'shaders/03_compute/main.comp.hlsl'
]

shaders = []
shaders += vert_shaders
shaders += frag_shaders
shaders += comp_shaders

foreach shader : shaders
    gen_spirv = run_command('glslangvalidator', '-e', 'main', '-V', meson.current_source_dir()/shader, '-o', meson.current_source_dir()/shader.replace('.hlsl', '.bin'), check: false)
    if gen_spirv.returncode() != 0
        output = gen_spirv.stdout().strip()
        errortxt = gen_spirv.stderr().strip()
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
