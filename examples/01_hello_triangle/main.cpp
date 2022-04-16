#define SAMPLE_NAME "01_hello_triangle"
#include "../common/sample.hpp"

DescriptorSetLayout* descriptor_set_layout;
Pipeline*            pipeline;

void init_sample()
{
    auto vert_code = read_shader( "main.vert" );
    auto frag_code = read_shader( "main.frag" );

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

    PipelineDesc pipeline_desc {};
    pipeline_desc.shader_count                 = 2;
    pipeline_desc.shaders[ 0 ]                 = shaders[ 0 ];
    pipeline_desc.shaders[ 1 ]                 = shaders[ 1 ];
    pipeline_desc.rasterizer_desc.cull_mode    = CullMode::eNone;
    pipeline_desc.rasterizer_desc.front_face   = FrontFace::eCounterClockwise;
    pipeline_desc.depth_state_desc.depth_test  = false;
    pipeline_desc.depth_state_desc.depth_write = false;
    pipeline_desc.descriptor_set_layout        = descriptor_set_layout;
    pipeline_desc.render_pass                  = render_passes[ 0 ];

    create_graphics_pipeline( device, &pipeline_desc, &pipeline );

    for ( u32 i = 0; i < 2; ++i ) { destroy_shader( device, shaders[ i ] ); }
}

void resize_sample( u32, u32 ) {}

void update_sample( CommandBuffer* cmd, f32 )
{
    u32 image_index = begin_frame();

    cmd_bind_pipeline( cmd, pipeline );
    cmd_draw( cmd, 3, 1, 0, 0 );

    end_frame( image_index );
}

void shutdown_sample()
{
    destroy_pipeline( device, pipeline );
    destroy_descriptor_set_layout( device, descriptor_set_layout );
}
