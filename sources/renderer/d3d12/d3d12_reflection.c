#ifdef D3D12_BACKEND
#include <d3d12.h>
#include <dxcapi.h>
#include <d3d12shader.h>
#include "renderer/d3d12/d3d12_backend.hpp"

namespace fluent
{

DescriptorType
to_descriptor_type( D3D_SHADER_INPUT_TYPE shader_input_type )
{
	switch ( shader_input_type )
	{
	case D3D_SIT_SAMPLER: return FT_DESCRIPTOR_TYPE_SAMPLER;
	case D3D_SIT_TEXTURE: return FT_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	case D3D_SIT_UAV_RWTYPED: return FT_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	case D3D_SIT_TBUFFER: return FT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
	case D3D_SIT_CBUFFER: return FT_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	case D3D_SIT_UAV_RWBYTEADDRESS: return FT_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	default: FT_ASSERT( false ); return DescriptorType( -1 );
	}
}

void
dxil_reflect_stage( ReflectionData* reflection_data,
                    ShaderStage     stage,
                    u32             byte_code_size,
                    const void*     byte_code )
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

	D3D12_SHADER_DESC info {};
	reflection->GetDesc( &info );

	reflection_data->binding_count += info.BoundResources;
	reflection_data->bindings.resize( reflection_data->binding_count );

	for ( u32 i = 0; i < info.BoundResources; ++i )
	{
		D3D12_SHADER_INPUT_BIND_DESC binding;
		reflection->GetResourceBindingDesc( i, &binding );

		reflection_data->bindings[ i ].binding          = binding.BindPoint;
		reflection_data->bindings[ i ].descriptor_count = binding.BindCount;
		reflection_data->bindings[ i ].descriptor_type =
		    to_descriptor_type( binding.Type );
		reflection_data->bindings[ i ].set = binding.Space;
	}

	reflection->Release();
	container_reflection->Release();
}

void
dxil_reflect( const Device* device, const ShaderInfo* info, Shader* shader )
{
	if ( info->vertex.bytecode )
	{
		dxil_reflect_stage( &shader->reflect_data,
		                    FT_SHADER_STAGE_VERTEX,
		                    info->vertex.bytecode_size,
		                    info->vertex.bytecode );
	}

	if ( info->fragment.bytecode )
	{
		dxil_reflect_stage( &shader->reflect_data,
		                    FT_SHADER_STAGE_FRAGMENT,
		                    info->fragment.bytecode_size,
		                    info->fragment.bytecode );
	}

	if ( info->compute.bytecode )
	{
		dxil_reflect_stage( &shader->reflect_data,
		                    FT_SHADER_STAGE_COMPUTE,
		                    info->compute.bytecode_size,
		                    info->compute.bytecode );
	}

	if ( info->tessellation_control.bytecode )
	{
		dxil_reflect_stage( &shader->reflect_data,
		                    FT_SHADER_STAGE_TESSELLATION_CONTROL,
		                    info->tessellation_control.bytecode_size,
		                    info->tessellation_control.bytecode );
	}

	if ( info->tessellation_evaluation.bytecode )
	{
		dxil_reflect_stage( &shader->reflect_data,
		                    FT_SHADER_STAGE_TESSELLATION_EVALUATION,
		                    info->tessellation_evaluation.bytecode_size,
		                    info->tessellation_evaluation.bytecode );
	}

	if ( info->geometry.bytecode )
	{
		dxil_reflect_stage( &shader->reflect_data,
		                    FT_SHADER_STAGE_GEOMETRY,
		                    info->geometry.bytecode_size,
		                    info->geometry.bytecode );
	}
}

} // namespace fluent
#endif
