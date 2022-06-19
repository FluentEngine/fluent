#ifdef METAL_BACKEND
#ifndef METAL_BACKEND_INCLUDE_OBJC
#define METAL_BACKEND_INCLUDE_OBJC
#endif
#include "../backend/metal/metal_backend.h"

FT_INLINE enum ft_descriptor_type
to_descriptor_type( MTLArgumentType type )
{
	switch ( type )
	{
	case MTLArgumentTypeBuffer: return FT_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	case MTLArgumentTypeSampler: return FT_DESCRIPTOR_TYPE_SAMPLER;
	case MTLArgumentTypeTexture: return FT_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	default: FT_ASSERT( false ); return ( enum ft_descriptor_type ) - 1;
	}
}

void
ft_mtl_reflect( const struct ft_device*      idevice,
                const struct ft_shader_info* info,
                struct ft_shader*            ishader )
{
#if 0
	@autoreleasepool
	{
		FT_FROM_HANDLE( device, idevice, MetalDevice );
		FT_FROM_HANDLE( shader, ishader, MetalShader );

		MTLRenderPipelineReflection* reflection;

		MTLVertexDescriptor* vertex_descriptor = [MTLVertexDescriptor new];
		for ( uint32_t i = 0; i < 10; ++i )
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
		    shader->shaders[ FT_SHADER_STAGE_VERTEX ];
		descriptor.fragmentFunction =
		    shader->shaders[ FT_SHADER_STAGE_FRAGMENT ];
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
				binding.binding          = arg.index;
				binding.set              = 0;
				binding.stage            = FT_SHADER_STAGE_VERTEX;

				binding_map[ [arg.name UTF8String] ] = binding_count;
				binding_count++;
			}
		}

		for ( MTLArgument* arg in reflection.fragmentArguments )
		{
			auto& binding            = bindings.emplace_back();
			binding.descriptor_count = arg.arrayLength;
			binding.descriptor_type  = to_descriptor_type( arg.type );
			binding.binding          = arg.index;
			binding.set              = 0;
			binding.stage            = FT_SHADER_STAGE_FRAGMENT;

			binding_map[ [arg.name UTF8String] ] = binding_count;
			binding_count++;
		}

		pipeline_state    = nil;
		reflection        = nil;
		descriptor        = nil;
		vertex_descriptor = nil;
	}
#endif
}

#endif
