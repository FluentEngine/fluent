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
    case MTLArgumentTypeBuffer: return DescriptorType::eUniformBuffer;
    case MTLArgumentTypeSampler: return DescriptorType::eSampler;
    case MTLArgumentTypeTexture: return DescriptorType::eSampledImage;
    default: FT_ASSERT( false ); return DescriptorType( -1 );
    }
}

void
mtl_reflect( const Device* idevice, const ShaderDesc* desc, Shader* ishader )
{
    FT_FROM_HANDLE( device, idevice, MetalDevice );
    FT_FROM_HANDLE( shader, ishader, MetalShader );

    MTLRenderPipelineReflection* reflection;

    MTLVertexDescriptor* vertex_descriptor = [MTLVertexDescriptor new];
    for ( u32 i = 0; i < 10; ++i )
    {
        vertex_descriptor.attributes[ i ].format      = MTLVertexFormatFloat;
        vertex_descriptor.attributes[ i ].bufferIndex = 0;
        vertex_descriptor.attributes[ i ].offset      = i * 4;
    }
    vertex_descriptor.layouts[ 0 ].stride = 10 * 4;
    vertex_descriptor.layouts[ 0 ].stepFunction =
        MTLVertexStepFunctionPerVertex;

    MTLRenderPipelineDescriptor* descriptor = [MTLRenderPipelineDescriptor new];
    descriptor.vertexDescriptor             = vertex_descriptor;
    descriptor.vertexFunction =
        shader->shaders[ static_cast<u32>( ShaderStage::eVertex ) ];
    descriptor.fragmentFunction =
        shader->shaders[ static_cast<u32>( ShaderStage::eFragment ) ];
    descriptor.colorAttachments[ 0 ].pixelFormat = MTLPixelFormatRGBA8Unorm;

    id pipeline_state = [device->device
        newRenderPipelineStateWithDescriptor:descriptor
                                     options:MTLPipelineOptionBufferTypeInfo
                                  reflection:&reflection
                                       error:nil];
    ( void ) pipeline_state;

    std::memset( shader->interface.reflect_data,
                 0,
                 sizeof( shader->interface.reflect_data ) );

    auto& vertex = shader->interface.reflect_data[ static_cast<u32>(
        ShaderStage::eVertex ) ];

    for ( MTLArgument* arg in reflection.vertexArguments )
    {
        if ( ![arg.name isEqualToString:@"vertexBuffer.0"] )
        {
            auto& binding            = vertex.bindings.emplace_back();
            binding.descriptor_count = arg.arrayLength;
            binding.descriptor_type  = to_descriptor_type( arg.type );
            binding.binding          = static_cast<u32>( arg.index );
            binding.set              = 0;

            vertex.binding_count++;
        }
    }

    auto& fragment = shader->interface.reflect_data[ static_cast<u32>(
        ShaderStage::eFragment ) ];

    for ( MTLArgument* arg in reflection.fragmentArguments )
    {
        auto& binding            = fragment.bindings.emplace_back();
        binding.descriptor_count = arg.arrayLength;
        binding.descriptor_type  = to_descriptor_type( arg.type );
        binding.binding          = static_cast<u32>( arg.index );
        binding.set              = 0;

        fragment.binding_count++;
    }

    [pipeline_state release];
    [reflection release];
    [descriptor release];
    [vertex_descriptor release];
}

} // namespace fluent
#endif
