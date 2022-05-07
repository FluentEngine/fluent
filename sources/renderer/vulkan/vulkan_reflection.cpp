#ifdef VULKAN_BACKEND
#include <spirv_reflect.h>
#include "renderer/vulkan/vulkan_backend.hpp"

namespace fluent
{

DescriptorType
to_descriptor_type( SpvReflectDescriptorType descriptor_type )
{
	switch ( descriptor_type )
	{
	case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: return DescriptorType::eSampler;
	case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		FT_ASSERT( false && "Use separate types instead, texture + sampler" );
		return DescriptorType( -1 );
	case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		return DescriptorType::eSampledImage;
	case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
		return DescriptorType::eStorageImage;
	case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
		return DescriptorType::eUniformTexelBuffer;
	case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
		return DescriptorType::eStorageTexelBuffer;
	case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		return DescriptorType::eUniformBuffer;
	case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		return DescriptorType::eStorageBuffer;
	case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
		return DescriptorType::eUniformBufferDynamic;
	case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
		return DescriptorType::eStorageBufferDynamic;
	case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
		return DescriptorType::eInputAttachment;
	default: FT_ASSERT( false ); return DescriptorType( -1 );
	}
}

void
spirv_reflect_stage( ReflectionData& reflection,
                     ShaderStage     stage,
                     u32             byte_code_size,
                     const void*     byte_code )
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
	                                           nullptr );
	FT_ASSERT( spv_result == SPV_REFLECT_RESULT_SUCCESS );
	std::vector<SpvReflectDescriptorBinding*> descriptor_bindings(
	    descriptor_binding_count );
	spv_result =
	    spvReflectEnumerateDescriptorBindings( &reflected_shader,
	                                           &descriptor_binding_count,
	                                           descriptor_bindings.data() );
	FT_ASSERT( spv_result == SPV_REFLECT_RESULT_SUCCESS );

	u32 i = reflection.bindings.size();
	reflection.binding_count += descriptor_binding_count;
	reflection.bindings.resize( reflection.bindings.size() +
	                            descriptor_binding_count );

	for ( const auto& descriptor_binding : descriptor_bindings )
	{
		reflection.bindings[ i ].binding          = descriptor_binding->binding;
		reflection.bindings[ i ].descriptor_count = descriptor_binding->count;
		reflection.bindings[ i ].descriptor_type =
		    to_descriptor_type( descriptor_binding->descriptor_type );
		reflection.bindings[ i ].set   = descriptor_binding->set;
		reflection.bindings[ i ].stage = stage;

		const char* name = descriptor_binding->type_description->type_name
		                       ? descriptor_binding->type_description->type_name
		                       : descriptor_binding->name;

		reflection.binding_map[ name ] = i;

		i++;
	}

	spvReflectDestroyShaderModule( &reflected_shader );
}

void
spirv_reflect( const Device* device, const ShaderInfo* info, Shader* shader )
{
	if ( info->vertex.bytecode )
	{
		spirv_reflect_stage( shader->reflect_data,
		                     ShaderStage::eVertex,
		                     info->vertex.bytecode_size,
		                     info->vertex.bytecode );
	}

	if ( info->fragment.bytecode )
	{
		spirv_reflect_stage( shader->reflect_data,
		                     ShaderStage::eFragment,
		                     info->fragment.bytecode_size,
		                     info->fragment.bytecode );
	}

	if ( info->compute.bytecode )
	{
		spirv_reflect_stage( shader->reflect_data,
		                     ShaderStage::eCompute,
		                     info->compute.bytecode_size,
		                     info->compute.bytecode );
	}

	if ( info->tessellation_control.bytecode )
	{
		spirv_reflect_stage( shader->reflect_data,
		                     ShaderStage::eTessellationControl,
		                     info->tessellation_control.bytecode_size,
		                     info->tessellation_control.bytecode );
	}

	if ( info->tessellation_evaluation.bytecode )
	{
		spirv_reflect_stage( shader->reflect_data,
		                     ShaderStage::eTessellationEvaluation,
		                     info->tessellation_evaluation.bytecode_size,
		                     info->tessellation_evaluation.bytecode );
	}

	if ( info->geometry.bytecode )
	{
		spirv_reflect_stage( shader->reflect_data,
		                     ShaderStage::eGeometry,
		                     info->geometry.bytecode_size,
		                     info->geometry.bytecode );
	}
}

} // namespace fluent
#endif
