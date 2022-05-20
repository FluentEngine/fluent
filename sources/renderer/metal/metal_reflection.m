#ifdef METAL_BACKEND
#define METAL_BACKEND_INCLUDE_OBJC
#include "renderer/metal/metal_backend.hpp"

namespace fluent
{

static inline DescriptorType
to_descriptor_type( MTLArgumentType type )
{
	switch ( type )
	{
	case MTLArgumentTypeBuffer: return DescriptorType::UNIFORM_BUFFER;
	case MTLArgumentTypeSampler: return DescriptorType::SAMPLER;
	case MTLArgumentTypeTexture: return DescriptorType::SAMPLED_IMAGE;
	default: FT_ASSERT( false ); return DescriptorType( -1 );
	}
}

void
mtl_reflect( const Device* idevice, const ShaderInfo* info, Shader* ishader )
{
	@autoreleasepool
	{
		FT_FROM_HANDLE( device, idevice, MetalDevice );
		FT_FROM_HANDLE( shader, ishader, MetalShader );

		MTLRenderPipelineReflection* reflection;

		MTLVertexDescriptor* vertex_descriptor = [MTLVertexDescriptor new];
		for ( u32 i = 0; i < 10; ++i )
		{
			vertex_descriptor.attributes[ i ].format = MTLVertexFormatFloat;
			vertex_descriptor.attributes[ i ].bufferIndex = 0;
			vertex_descriptor.attributes[ i ].offset      = i * 4;
		}
		vertex_descriptor.layouts[ 0 ].stride = 10 * 4;
		vertex_descriptor.layouts[ 0 ].stepFunction =
		    MTLVertexStepFunctionPerVertex;

		MTLRenderPipelineDescriptor* descriptor =
		    [MTLRenderPipelineDescriptor new];
		descriptor.vertexDescriptor = vertex_descriptor;
		descriptor.vertexFunction =
		    shader->shaders[ static_cast<u32>( ShaderStage::VERTEX ) ];
		descriptor.fragmentFunction =
		    shader->shaders[ static_cast<u32>( ShaderStage::FRAGMENT ) ];
		descriptor.colorAttachments[ 0 ].pixelFormat = MTLPixelFormatRGBA8Unorm;

		id pipeline_state = [device->device
		    newRenderPipelineStateWithDescriptor:descriptor
		                                 options:MTLPipelineOptionBufferTypeInfo
		                              reflection:&reflection
		                                   error:nil];
		( void ) pipeline_state;

		auto& binding_map   = shader->interface.reflect_data.binding_map;
		auto& binding_count = shader->interface.reflect_data.binding_count;
		auto& bindings      = shader->interface.reflect_data.bindings;

		for ( MTLArgument* arg in reflection.vertexArguments )
		{
			if ( ![arg.name isEqualToString:@"vertexBuffer.0"] )
			{
				auto& binding            = bindings.emplace_back();
				binding.descriptor_count = arg.arrayLength;
				binding.descriptor_type  = to_descriptor_type( arg.type );
				binding.binding          = static_cast<u32>( arg.index );
				binding.set              = 0;
				binding.stage            = ShaderStage::VERTEX;

				binding_map[ [arg.name UTF8String] ] = binding_count;
				binding_count++;
			}
		}

		for ( MTLArgument* arg in reflection.fragmentArguments )
		{
			auto& binding            = bindings.emplace_back();
			binding.descriptor_count = arg.arrayLength;
			binding.descriptor_type  = to_descriptor_type( arg.type );
			binding.binding          = static_cast<u32>( arg.index );
			binding.set              = 0;
			binding.stage            = ShaderStage::FRAGMENT;

			binding_map[ [arg.name UTF8String] ] = binding_count;
			binding_count++;
		}

		pipeline_state    = nil;
		reflection        = nil;
		descriptor        = nil;
		vertex_descriptor = nil;
	}
}

} // namespace fluent
#endif
