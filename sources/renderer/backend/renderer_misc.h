#pragma once

#define FT_DECLARE_SHADER( name )                                              \
	FT_INLINE struct ft_shader_module_info get_##name##_shader(                \
	    enum ft_renderer_api api )                                             \
	{                                                                          \
		struct ft_shader_module_info info;                                     \
		switch ( api )                                                         \
		{                                                                      \
		case FT_RENDERER_API_VULKAN:                                           \
		{                                                                      \
			info.bytecode_size = shader_##name##_spirv_len;                    \
			info.bytecode      = shader_##name##_spirv;                        \
			break;                                                             \
		}                                                                      \
		case FT_RENDERER_API_METAL:                                            \
		{                                                                      \
		}                                                                      \
		case FT_RENDERER_API_D3D12:                                            \
		{                                                                      \
		}                                                                      \
		}                                                                      \
		return info;                                                           \
	}
