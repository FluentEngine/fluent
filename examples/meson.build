examples = [ 
    '01_hello_triangle',
    '02_load_texture',
    '03_compute'
]

shaders = {
    examples[0] : [
        'shaders/01_hello_triangle/main.vert.hlsl',
        'shaders/01_hello_triangle/main.frag.hlsl',
    ],
    examples[1] : [
        'shaders/02_load_texture/main.vert.hlsl',
        'shaders/02_load_texture/main.frag.hlsl',
    ],
    examples[2] : [
        'shaders/03_compute/main.comp.hlsl'
    ]
}

prog_python = import('python').find_installation('python3')

bytecodes = []
if renderer_backend_vulkan
    bytecodes += [ 'spirv' ]
endif

if renderer_backend_d3d12
    bytecodes += [ 'dxil' ]
endif

foreach example : examples
    srcs = files([
        example/'main.cpp',
        'common/sample.hpp'
    ])

    example_shaders = shaders[example]
    compiled_example_shaders = []
    foreach s : example_shaders
        shader_bytecode_gen = generator(prog_python,
                                        output : '@BASENAME@.bin',
                                        arguments : [ 
                                            meson.current_source_dir()/'../tools/compile_shaders.py', 
                                            '--outdir', meson.current_source_dir()/'shaders'/example, 
                                            '--bytecodes', [ bytecodes ], 
                                            '--shader', '@INPUT@' ])

        compiled_example_shaders += shader_bytecode_gen.process(s)
    endforeach

    executable(example, srcs, compiled_example_shaders,
        dependencies	: fluent_engine_dep,
        cpp_args		: compile_args,
    )
endforeach
