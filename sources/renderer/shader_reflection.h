#pragma once

#include "base/base.h"
#include "renderer/renderer_enums.h"
#include "renderer/renderer_backend.h"

#ifdef __cplusplus
extern "C"
{
#endif

	struct Device;
	struct Shader;
	struct ShaderInfo;

	typedef struct Binding
	{
		u32            set;
		u32            binding;
		u32            descriptor_count;
		DescriptorType descriptor_type;
		ShaderStage    stage;
	} Binding;

	typedef Binding* Bindings;
	typedef struct BindingMap
	{
		const char* key;
		u32         value;
	} BindingMap;

	typedef struct ReflectionData
	{
		u32        binding_count;
		Bindings   bindings;
		BindingMap binding_map;
	} ReflectionData;

	void
	dxil_reflect( const struct Device*     device,
	              const struct ShaderInfo* info,
	              struct Shader*           shader );

	void
	spirv_reflect( const struct Device*     device,
	               const struct ShaderInfo* info,
	               struct Shader*           shader );

	void
	mtl_reflect( const struct Device*     device,
	             const struct ShaderInfo* info,
	             struct Shader*           shader );
#ifdef __cplusplus
}
#endif
