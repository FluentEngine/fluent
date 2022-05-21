#pragma once

#include "base/base.h"
#include "renderer/renderer_enums.h"
#include "renderer/renderer_backend.h"

#define MAX_BINDING_NAME_LENGTH 20

#ifdef __cplusplus
extern "C"
{
#endif

	struct Device;
	struct Shader;
	struct ShaderInfo;

	struct Binding
	{
		u32                 set;
		u32                 binding;
		u32                 descriptor_count;
		enum DescriptorType descriptor_type;
		enum ShaderStage    stage;
	};

	typedef struct Binding* Bindings;

	struct BindingMapItem
	{
		char name[ MAX_BINDING_NAME_LENGTH ];
		u32  value;
	};

	typedef struct ReflectionData
	{
		u32             binding_count;
		Bindings        bindings;
		struct hashmap* binding_map;
	} ReflectionData;

	i32
	binding_map_compare( const void* a, const void* b, void* udata );

	u64
	binding_map_hash( const void* item, u64 seed0, u64 seed1 );

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
