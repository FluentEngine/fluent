#pragma once

#define DECLARE_SHADER( name )                                                 \
	static inline struct ShaderModuleInfo get_##name##_shader(                 \
	    enum RendererAPI api )                                                 \
	{                                                                          \
		struct ShaderModuleInfo info;                                          \
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
