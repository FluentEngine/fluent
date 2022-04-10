examples = [ 
	'01_hello_triangle',
	'02_load_texture',
	'03_compute'
]

shaders = {
	'01_hello_triangle': [
		'shaders/01_hello_triangle/main.vert.hlsl',
		'shaders/01_hello_triangle/main.frag.hlsl',
	],
	'02_load_texture': files([
		'shaders/02_load_texture/main.vert.hlsl',
		'shaders/02_load_texture/main.frag.hlsl',
	]),
	'03_compute': [
		'shaders/03_compute/main.comp.hlsl'
	]
}

foreach example : examples
	srcs = [
		example/'main.cpp',
		'common/sample.hpp'
	]
		executable(example, srcs, glsl_generator.process(shaders[example]),
		dependencies	: fluent_engine_dep, 
		cpp_args		: compile_args,
		)
endforeach