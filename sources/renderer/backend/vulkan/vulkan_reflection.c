#include "base/base.h"

#if FT_VULKAN_BACKEND
#include <spirv_reflect/spirv_reflect.h>
#include <hashmap_c/hashmap_c.h>
#include "renderer/backend/renderer_private.h"
#include "vulkan_backend.h"

int32_t
binding_map_compare( const void* a, const void* b, void* udata )
{
	const struct ft_binding_map_item* bma = a;
	const struct ft_binding_map_item* bmb = b;
	return strcmp( bma->name, bmb->name );
}

uint64_t
binding_map_hash( const void* item, uint64_t seed0, uint64_t seed1 )
{
	const struct ft_binding_map_item* map = item;
	return hashmap_sip( map->name, strlen( map->name ), seed0, seed1 );
}

static enum ft_descriptor_type
to_descriptor_type( SpvReflectDescriptorType descriptor_type )
{
	switch ( descriptor_type )
	{
	case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: return FT_DESCRIPTOR_TYPE_SAMPLER;
	case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		FT_ASSERT( 0 && "Use separate types instead, texture + sampler" );
		return ( enum ft_descriptor_type ) - 1;
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
	default: FT_ASSERT( false ); return ( enum ft_descriptor_type ) - 1;
	}
}

static void
spirv_reflect_stage( struct ft_reflection_data* reflection,
                     enum ft_shader_stage       stage,
                     uint32_t                   byte_code_size,
                     const void*                byte_code )
{
	SpvReflectResult       spv_result;
	SpvReflectShaderModule reflected_shader;
	spv_result = spvReflectCreateShaderModule( byte_code_size,
	                                           byte_code,
	                                           &reflected_shader );
	FT_ASSERT( spv_result == SPV_REFLECT_RESULT_SUCCESS );

	uint32_t descriptor_binding_count = 0;
	spv_result =
	    spvReflectEnumerateDescriptorBindings( &reflected_shader,
	                                           &descriptor_binding_count,
	                                           NULL );
	FT_ASSERT( spv_result == SPV_REFLECT_RESULT_SUCCESS );

	FT_ALLOC_STACK_ARRAY( SpvReflectDescriptorBinding*,
	                      descriptor_bindings,
	                      descriptor_binding_count );
	spv_result =
	    spvReflectEnumerateDescriptorBindings( &reflected_shader,
	                                           &descriptor_binding_count,
	                                           descriptor_bindings );
	FT_ASSERT( spv_result == SPV_REFLECT_RESULT_SUCCESS );

	uint32_t i = reflection->binding_count;
	reflection->binding_count += descriptor_binding_count;
	if ( reflection->binding_count != 0 )
	{
		reflection->bindings =
		    realloc( reflection->bindings,
		             sizeof( struct ft_binding ) * reflection->binding_count );
	}

	for ( uint32_t b = 0; b < descriptor_binding_count; ++b )
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

		struct ft_binding_map_item item;
		item.value = i;
		memset( item.name, '\0', FT_MAX_BINDING_NAME_LENGTH );
		strcpy( item.name, name );
		hashmap_set( reflection->binding_map, &item );
		i++;
	}

	spvReflectDestroyShaderModule( &reflected_shader );
}

void
spirv_reflect( const struct ft_device*      device,
               const struct ft_shader_info* info,
               struct ft_shader*            shader )
{
	shader->reflect_data.binding_map =
	    hashmap_new( sizeof( struct ft_binding_map_item ),
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
