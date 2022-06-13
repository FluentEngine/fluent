#ifdef VULKAN_BACKEND
#include <spirv_reflect/spirv_reflect.h>
#include <hashmap_c/hashmap_c.h>
#include "base/base.h"
#include "log/log.h"
#include "renderer/backend/vulkan/vulkan_backend.h"

i32
binding_map_compare( const void* a, const void* b, void* udata )
{
	const struct BindingMapItem* bma = a;
	const struct BindingMapItem* bmb = b;
	return strcmp( bma->name, bmb->name );
}

u64
binding_map_hash( const void* item, u64 seed0, u64 seed1 )
{
	const struct BindingMapItem* map = item;
	return hashmap_sip( map->name, strlen( map->name ), seed0, seed1 );
}

static enum DescriptorType
to_descriptor_type( SpvReflectDescriptorType descriptor_type )
{
	switch ( descriptor_type )
	{
	case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: return FT_DESCRIPTOR_TYPE_SAMPLER;
	case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		FT_ASSERT( 0 && "Use separate types instead, texture + sampler" );
		return ( enum DescriptorType ) - 1;
	case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		return FT_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
		return FT_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
		return FT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
	case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
		return FT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
	case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		return FT_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		return FT_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
		return FT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
		return FT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
	case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
		return FT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	default: FT_ASSERT( 0 ); return ( enum DescriptorType ) - 1;
	}
}

static void
spirv_reflect_stage( ReflectionData*  reflection,
                     enum ShaderStage stage,
                     u32              byte_code_size,
                     const void*      byte_code )
{
	SpvReflectResult       spv_result;
	SpvReflectShaderModule reflected_shader;
	spv_result = spvReflectCreateShaderModule( byte_code_size,
	                                           byte_code,
	                                           &reflected_shader );
	FT_ASSERT( spv_result == SPV_REFLECT_RESULT_SUCCESS );

	u32 descriptor_binding_count = 0;
	spv_result =
	    spvReflectEnumerateDescriptorBindings( &reflected_shader,
	                                           &descriptor_binding_count,
	                                           NULL );
	FT_ASSERT( spv_result == SPV_REFLECT_RESULT_SUCCESS );

	ALLOC_STACK_ARRAY( SpvReflectDescriptorBinding*,
	                   descriptor_bindings,
	                   descriptor_binding_count );
	spv_result =
	    spvReflectEnumerateDescriptorBindings( &reflected_shader,
	                                           &descriptor_binding_count,
	                                           descriptor_bindings );
	FT_ASSERT( spv_result == SPV_REFLECT_RESULT_SUCCESS );

	u32 i = reflection->binding_count;
	reflection->binding_count += descriptor_binding_count;
	if ( reflection->binding_count != 0 )
	{
		reflection->bindings =
		    realloc( reflection->bindings,
		             sizeof( struct Binding ) * reflection->binding_count );
	}

	for ( u32 b = 0; b < descriptor_binding_count; ++b )
	{
		reflection->bindings[ i ].binding = descriptor_bindings[ b ]->binding;
		reflection->bindings[ i ].descriptor_count =
		    descriptor_bindings[ b ]->count;
		reflection->bindings[ i ].descriptor_type =
		    to_descriptor_type( descriptor_bindings[ b ]->descriptor_type );
		reflection->bindings[ i ].set   = descriptor_bindings[ b ]->set;
		reflection->bindings[ i ].stage = stage;

		const char* name =
		    descriptor_bindings[ b ]->type_description->type_name
		        ? descriptor_bindings[ b ]->type_description->type_name
		        : descriptor_bindings[ b ]->name;

		struct BindingMapItem item;
		item.value = i;
		memset( item.name, '\0', MAX_BINDING_NAME_LENGTH );
		strcpy( item.name, name );
		hashmap_set( reflection->binding_map, &item );
		i++;
	}

	spvReflectDestroyShaderModule( &reflected_shader );
}

void
spirv_reflect( const struct Device*     device,
               const struct ShaderInfo* info,
               struct Shader*           shader )
{
	shader->reflect_data.binding_map =
	    hashmap_new( sizeof( struct BindingMapItem ),
	                 0,
	                 0,
	                 0,
	                 binding_map_hash,
	                 binding_map_compare,
	                 NULL,
	                 NULL );

	if ( info->vertex.bytecode )
	{
		spirv_reflect_stage( &shader->reflect_data,
		                     FT_SHADER_STAGE_VERTEX,
		                     info->vertex.bytecode_size,
		                     info->vertex.bytecode );
	}

	if ( info->fragment.bytecode )
	{
		spirv_reflect_stage( &shader->reflect_data,
		                     FT_SHADER_STAGE_FRAGMENT,
		                     info->fragment.bytecode_size,
		                     info->fragment.bytecode );
	}

	if ( info->compute.bytecode )
	{
		spirv_reflect_stage( &shader->reflect_data,
		                     FT_SHADER_STAGE_COMPUTE,
		                     info->compute.bytecode_size,
		                     info->compute.bytecode );
	}

	if ( info->tessellation_control.bytecode )
	{
		spirv_reflect_stage( &shader->reflect_data,
		                     FT_SHADER_STAGE_TESSELLATION_CONTROL,
		                     info->tessellation_control.bytecode_size,
		                     info->tessellation_control.bytecode );
	}

	if ( info->tessellation_evaluation.bytecode )
	{
		spirv_reflect_stage( &shader->reflect_data,
		                     FT_SHADER_STAGE_TESSELLATION_EVALUATION,
		                     info->tessellation_evaluation.bytecode_size,
		                     info->tessellation_evaluation.bytecode );
	}

	if ( info->geometry.bytecode )
	{
		spirv_reflect_stage( &shader->reflect_data,
		                     FT_SHADER_STAGE_GEOMETRY,
		                     info->geometry.bytecode_size,
		                     info->geometry.bytecode );
	}
}

#endif
