sources = files([
	'../base/base.h',
	'log.h',
	'log.c'
])

ft_log_lib = library('ft_log', sources,
	include_directories : include_directories('../'),
	c_args : ft_c_args)

ft_log_dep = declare_dependency(include_directories : include_directories('../'), link_with : ft_log_lib)
