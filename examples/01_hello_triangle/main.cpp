#define SAMPLE_NAME "01_hello_triangle"
#include "../common/sample.hpp"

DescriptorSetLayout* descriptor_set_layout;
Pipeline*            pipeline;

Buffer* vertex_buffer;

static const f32 vertices[] = { -0.5f, -0.5f, 0.5f, -0.5f, 0.0f, 0.5f };

void init_sample()
{
    auto vert_code = read_file_binary( FileSystem::get_shaders_directory() +
                                       "/main.vert.glsl.spv" );
    auto frag_code = read_file_binary( FileSystem::get_shaders_directory() +
                                       "/main.frag.glsl.spv" );

    ShaderDesc shader_descs[ 2 ];
    shader_descs[ 0 ].stage = ShaderStage::eVertex;
    shader_descs[ 0 ].bytecode_size =
        vert_code.size() * sizeof( vert_code[ 0 ] );
    shader_descs[ 0 ].bytecode = vert_code.data();
    shader_descs[ 1 ].stage    = ShaderStage::eFragment;
    shader_descs[ 1 ].bytecode_size =
        frag_code.size() * sizeof( frag_code[ 0 ] );
    shader_descs[ 1 ].bytecode = frag_code.data();

    Shader* shaders[ 2 ] = {};
    create_shader( device, &shader_descs[ 0 ], &shaders[ 0 ] );
    create_shader( device, &shader_descs[ 1 ], &shaders[ 1 ] );

    create_descriptor_set_layout( device, 2, shaders, &descriptor_set_layout );

    PipelineDesc  pipeline_desc {};
    VertexLayout& vertex_layout                 = pipeline_desc.vertex_layout;
    vertex_layout.binding_desc_count            = 1;
    vertex_layout.binding_descs[ 0 ].binding    = 0;
    vertex_layout.binding_descs[ 0 ].input_rate = VertexInputRate::eVertex;
    vertex_layout.binding_descs[ 0 ].stride     = 2 * sizeof( float );
    vertex_layout.attribute_desc_count          = 1;
    vertex_layout.attribute_descs[ 0 ].binding  = 0;
    vertex_layout.attribute_descs[ 0 ].format   = Format::eR32G32Sfloat;
    vertex_layout.attribute_descs[ 0 ].location = 0;
    vertex_layout.attribute_descs[ 0 ].offset   = 0;
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
    buffer_desc.memory_usage    = MemoryUsage::eCpuToGpu;

    create_buffer( device, &buffer_desc, &vertex_buffer );
    map_memory( device, vertex_buffer );
    std::memcpy( vertex_buffer->mapped_memory, vertices, sizeof( vertices ) );
    unmap_memory( device, vertex_buffer );
}

void resize_sample( u32 width, u32 height ) {}

void update_sample( CommandBuffer* cmd, f32 delta_time )
{
    u32 image_index = begin_frame();

    cmd_bind_pipeline( cmd, pipeline );
    cmd_bind_vertex_buffer( cmd, vertex_buffer, 0 );
    cmd_draw( cmd, 3, 1, 0, 0 );

    end_frame( image_index );
}

void shutdown_sample()
{
    destroy_buffer( device, vertex_buffer );
    destroy_pipeline( device, pipeline );
    destroy_descriptor_set_layout( device, descriptor_set_layout );
}
