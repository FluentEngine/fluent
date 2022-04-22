#ifdef VULKAN_BACKEND
#include <spirv_reflect.h>
#endif
#ifdef D3D12_BACKEND
#include <d3d12.h>
#include <dxcapi.h>
#include <d3d12shader.h>
#endif
#include "renderer/shader_reflection.hpp"

namespace fluent
{
#ifdef VULKAN_BACKEND
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
spirv_reflect( u32 byte_code_size, const void* byte_code )
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
#endif

#ifdef D3D12_BACKEND
DescriptorType
to_descriptor_type( D3D_SHADER_INPUT_TYPE shader_input_type )
{
    switch ( shader_input_type )
    {
    case D3D_SIT_SAMPLER: return DescriptorType::eSampler;
    case D3D_SIT_TEXTURE: return DescriptorType::eSampledImage;
    case D3D_SIT_UAV_RWTYPED: return DescriptorType::eStorageImage;
    case D3D_SIT_TBUFFER: return DescriptorType::eUniformTexelBuffer;
    case D3D_SIT_CBUFFER: return DescriptorType::eUniformBuffer;
    case D3D_SIT_UAV_RWBYTEADDRESS: return DescriptorType::eStorageBuffer;
    default: FT_ASSERT( false ); return DescriptorType( -1 );
    }
}

ReflectionData
dxil_reflect( u32 byte_code_size, const void* byte_code )
{
#define DXIL_FOURCC( ch0, ch1, ch2, ch3 )                                      \
    ( ( uint32_t ) ( uint8_t ) ( ch0 ) |                                       \
      ( uint32_t ) ( uint8_t ) ( ch1 ) << 8 |                                  \
      ( uint32_t ) ( uint8_t ) ( ch2 ) << 16 |                                 \
      ( uint32_t ) ( uint8_t ) ( ch3 ) << 24 )

    HRESULT      result;
    IDxcLibrary* library;
    result = DxcCreateInstance( CLSID_DxcLibrary, IID_PPV_ARGS( &library ) );
    FT_ASSERT( result == S_OK );
    IDxcBlobEncoding* blob;
    library->CreateBlobWithEncodingFromPinned( byte_code,
                                               byte_code_size,
                                               0,
                                               &blob );
    IDxcContainerReflection* container_reflection;
    ID3D12ShaderReflection*  reflection;
    u32                      shader_idx;
    DxcCreateInstance( CLSID_DxcContainerReflection,
                       IID_PPV_ARGS( &container_reflection ) );
    container_reflection->Load( blob );
    result = ( container_reflection->FindFirstPartKind(
        DXIL_FOURCC( 'D', 'X', 'I', 'L' ),
        &shader_idx ) );
    FT_ASSERT( result == S_OK );
    result = ( container_reflection->GetPartReflection(
        shader_idx,
        IID_PPV_ARGS( &reflection ) ) );
    FT_ASSERT( result == S_OK );

    ReflectionData reflection_data {};

    D3D12_SHADER_DESC desc {};
    reflection->GetDesc( &desc );

    reflection_data.binding_count = desc.BoundResources;
    reflection_data.bindings.resize( reflection_data.binding_count );

    for ( u32 i = 0; i < desc.BoundResources; ++i )
    {
        D3D12_SHADER_INPUT_BIND_DESC binding;
        reflection->GetResourceBindingDesc( i, &binding );

        reflection_data.bindings[ i ].binding          = binding.BindPoint;
        reflection_data.bindings[ i ].descriptor_count = binding.BindCount;
        reflection_data.bindings[ i ].descriptor_type =
            to_descriptor_type( binding.Type );
        reflection_data.bindings[ i ].set = binding.Space;
    }

    reflection->Release();
    container_reflection->Release();
    return reflection_data;
}
#endif
} // namespace fluent
