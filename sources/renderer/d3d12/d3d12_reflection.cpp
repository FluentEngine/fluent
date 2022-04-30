#ifdef D3D12_BACKEND
#include <d3d12.h>
#include <dxcapi.h>
#include <d3d12shader.h>
#include "renderer/shader_reflection.hpp"

namespace fluent
{

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
dxil_reflect_stage( u32 byte_code_size, const void* byte_code )
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

void
dxil_reflect( const Device* device, const ShaderDesc* desc, Shader* shader )
{
    if ( desc->vertex.bytecode )
    {
        shader->reflect_data[ static_cast<u32>( ShaderStage::eVertex ) ] =
            dxil_reflect_stage( desc->vertex.bytecode_size,
                                desc->vertex.bytecode );
    }

    if ( desc->fragment.bytecode )
    {
        shader->reflect_data[ static_cast<u32>( ShaderStage::eFragment ) ] =
            dxil_reflect_stage( desc->fragment.bytecode_size,
                                desc->fragment.bytecode );
    }

    if ( desc->compute.bytecode )
    {
        shader->reflect_data[ static_cast<u32>( ShaderStage::eCompute ) ] =
            dxil_reflect_stage( desc->compute.bytecode_size,
                                desc->compute.bytecode );
    }

    if ( desc->tessellation_control.bytecode )
    {
        shader->reflect_data[ static_cast<u32>(
            ShaderStage::eTessellationControl ) ] =
            dxil_reflect_stage( desc->tessellation_control.bytecode_size,
                                desc->tessellation_control.bytecode );
    }

    if ( desc->tessellation_evaluation.bytecode )
    {
        shader->reflect_data[ static_cast<u32>(
            ShaderStage::eTessellationEvaluation ) ] =
            dxil_reflect_stage( desc->tessellation_evaluation.bytecode_size,
                                desc->tessellation_evaluation.bytecode );
    }

    if ( desc->geometry.bytecode )
    {
        shader->reflect_data[ static_cast<u32>( ShaderStage::eGeometry ) ] =
            dxil_reflect_stage( desc->geometry.bytecode_size,
                                desc->geometry.bytecode );
    }
}

} // namespace fluent
#endif