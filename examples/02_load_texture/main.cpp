#define SAMPLE_NAME "02_load_texture"
#include "../common/sample.hpp"

DescriptorSetLayout* descriptor_set_layout;
Pipeline*            pipeline;

Buffer* vertex_buffer;

// clang-format off
static const f32 vertices[] = {
    -0.5f, -0.5f, 0.0f, 0.0f, 
    0.5f, -0.5f, 1.0f, 0.0f, 
    0.5f, 0.5f, 1.0f, 1.0f, 
    0.5f, 0.5f, 1.0f, 1.0f, 
    -0.5f, 0.5f, 0.0f, 1.0f,
    -0.5f, -0.5f, 0.0f, 0.0f,
};
// clang-format on

DescriptorSet* descriptor_set;
Sampler*       sampler;
Image*         texture;

void init_sample()
{
    Shader* shaders[ 2 ];
    shaders[ 0 ] = load_shader_from_file( "main.vert" );
    shaders[ 1 ] = load_shader_from_file( "main.frag" );

    create_descriptor_set_layout( device, 2, shaders, &descriptor_set_layout );

    PipelineDesc  pipeline_desc {};
    VertexLayout& vertex_layout                 = pipeline_desc.vertex_layout;
    vertex_layout.binding_desc_count            = 1;
    vertex_layout.binding_descs[ 0 ].binding    = 0;
    vertex_layout.binding_descs[ 0 ].input_rate = VertexInputRate::eVertex;
    vertex_layout.binding_descs[ 0 ].stride     = 4 * sizeof( float );
    vertex_layout.attribute_desc_count          = 2;
    vertex_layout.attribute_descs[ 0 ].binding  = 0;
    vertex_layout.attribute_descs[ 0 ].format   = Format::eR32G32Sfloat;
    vertex_layout.attribute_descs[ 0 ].location = 0;
    vertex_layout.attribute_descs[ 0 ].offset   = 0;
    vertex_layout.attribute_descs[ 1 ].binding  = 0;
    vertex_layout.attribute_descs[ 1 ].format   = Format::eR32G32Sfloat;
    vertex_layout.attribute_descs[ 1 ].location = 1;
    vertex_layout.attribute_descs[ 1 ].offset   = 2 * sizeof( float );
    pipeline_desc.shader_count                  = 2;
    pipeline_desc.shaders[ 0 ]                  = shaders[ 0 ];
    pipeline_desc.shaders[ 1 ]                  = shaders[ 1 ];
    pipeline_desc.rasterizer_desc.cull_mode     = CullMode::eNone;
    pipeline_desc.rasterizer_desc.front_face    = FrontFace::eCounterClockwise;
    pipeline_desc.depth_state_desc.depth_test   = false;
    pipeline_desc.depth_state_desc.depth_write  = false;
    pipeline_desc.descriptor_set_layout         = descriptor_set_layout;
	pipeline_desc.render_pass                   = render_passes[ 0 ];

    create_graphics_pipeline( device, &pipeline_desc, &pipeline );

    for ( u32 i = 0; i < 2; ++i ) { destroy_shader( device, shaders[ i ] ); }

    BufferDesc buffer_desc {};
    buffer_desc.size            = sizeof( vertices );
    buffer_desc.descriptor_type = DescriptorType::eVertexBuffer;

    create_buffer( device, &buffer_desc, &vertex_buffer );

    ResourceLoader::upload_buffer( vertex_buffer,
                                   0,
                                   sizeof( vertices ),
                                   vertices );

    SamplerDesc sampler_desc {};
    sampler_desc.mipmap_mode = SamplerMipmapMode::eLinear;
    sampler_desc.min_lod     = 0;
    sampler_desc.max_lod     = 1000;

    create_sampler( device, &sampler_desc, &sampler );

    texture = load_image_from_file( "statue.jpg", true );

    ImageDescriptor image_descriptor {};
    image_descriptor.image          = texture;
    image_descriptor.resource_state = ResourceState::eShaderReadOnly;

    ImageDescriptor sampler_descriptor {};
    sampler_descriptor.sampler = sampler;

    DescriptorWrite descriptor_writes[ 2 ]   = {};
    descriptor_writes[ 0 ].descriptor_type   = DescriptorType::eSampler;
    descriptor_writes[ 0 ].binding           = 0;
    descriptor_writes[ 0 ].descriptor_count  = 1;
    descriptor_writes[ 0 ].image_descriptors = &sampler_descriptor;
    descriptor_writes[ 1 ].descriptor_type   = DescriptorType::eSampledImage;
    descriptor_writes[ 1 ].binding           = 1;
    descriptor_writes[ 1 ].descriptor_count  = 1;
    descriptor_writes[ 1 ].image_descriptors = &image_descriptor;

    DescriptorSetDesc descriptor_set_desc {};
    descriptor_set_desc.set                   = 0;
    descriptor_set_desc.descriptor_set_layout = descriptor_set_layout;

    create_descriptor_set( device, &descriptor_set_desc, &descriptor_set );
    update_descriptor_set( device, descriptor_set, 2, descriptor_writes );
}

void resize_sample( u32, u32 ) {}

void update_sample( CommandBuffer* cmd, f32 )
{
    u32 image_index = begin_frame();

    cmd_bind_pipeline( cmd, pipeline );
    cmd_bind_descriptor_set( cmd, 0, descriptor_set, pipeline );
    cmd_bind_vertex_buffer( cmd, vertex_buffer, 0 );
    cmd_draw( cmd, 6, 1, 0, 0 );

    end_frame( image_index );
}

void shutdown_sample()
{
    destroy_descriptor_set( device, descriptor_set );
    destroy_image( device, texture );
    destroy_sampler( device, sampler );
    destroy_buffer( device, vertex_buffer );
    destroy_pipeline( device, pipeline );
    destroy_descriptor_set_layout( device, descriptor_set_layout );
}
