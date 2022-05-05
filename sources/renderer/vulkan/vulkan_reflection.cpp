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

ReflectionData
spirv_reflect_stage( u32 byte_code_size, const void* byte_code )
{
    ReflectionData result {};

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

    result.binding_count = descriptor_binding_count;
    result.bindings.resize( result.binding_count );

    u32 i = 0;
    for ( const auto& descriptor_binding : descriptor_bindings )
    {
        result.bindings[ i ].binding          = descriptor_binding->binding;
        result.bindings[ i ].descriptor_count = descriptor_binding->count;
        result.bindings[ i ].descriptor_type =
            to_descriptor_type( descriptor_binding->descriptor_type );
        result.bindings[ i ].set = descriptor_binding->set;
        i++;
    }

    spvReflectDestroyShaderModule( &reflected_shader );

    return result;
}

void
spirv_reflect( const Device* device, const ShaderInfo* info, Shader* shader )
{
    if ( info->vertex.bytecode )
    {
        shader->reflect_data[ static_cast<u32>( ShaderStage::eVertex ) ] =
            spirv_reflect_stage( info->vertex.bytecode_size,
                                 info->vertex.bytecode );
    }

    if ( info->fragment.bytecode )
    {
        shader->reflect_data[ static_cast<u32>( ShaderStage::eFragment ) ] =
            spirv_reflect_stage( info->fragment.bytecode_size,
                                 info->fragment.bytecode );
    }

    if ( info->compute.bytecode )
    {
        shader->reflect_data[ static_cast<u32>( ShaderStage::eCompute ) ] =
            spirv_reflect_stage( info->compute.bytecode_size,
                                 info->compute.bytecode );
    }

    if ( info->tessellation_control.bytecode )
    {
        shader->reflect_data[ static_cast<u32>(
            ShaderStage::eTessellationControl ) ] =
            spirv_reflect_stage( info->tessellation_control.bytecode_size,
                                 info->tessellation_control.bytecode );
    }

    if ( info->tessellation_evaluation.bytecode )
    {
        shader->reflect_data[ static_cast<u32>(
            ShaderStage::eTessellationEvaluation ) ] =
            spirv_reflect_stage( info->tessellation_evaluation.bytecode_size,
                                 info->tessellation_evaluation.bytecode );
    }

    if ( info->geometry.bytecode )
    {
        shader->reflect_data[ static_cast<u32>( ShaderStage::eGeometry ) ] =
            spirv_reflect_stage( info->geometry.bytecode_size,
                                 info->geometry.bytecode );
    }
}

} // namespace fluent
#endif
