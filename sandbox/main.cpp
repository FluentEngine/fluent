#include <iostream>

#include "fluent/fluent.hpp"

using namespace fluent;

static constexpr u32 SCREEN_WIDTH  = 1400;
static constexpr u32 SCREEN_HEIGHT = 900;

DescriptorSetLayout* dsl;
Pipeline*            pipeline;

Buffer* vertex_buffer;

static const f32 vertices[] = { -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.0f, 0.5f, 0.0f };

void on_init()
{
    FileSystem::set_shaders_directory("../../sandbox/shaders/");
    FileSystem::set_models_directory("../../sandbox/models/");
    FileSystem::set_textures_directory("../../sandbox/textures/");

    GraphicContextDesc desc{};
    desc.width         = SCREEN_WIDTH;
    desc.height        = SCREEN_HEIGHT;
    desc.builtin_depth = true;
    GraphicContext::init(desc);

    ResourceLoader::init(GraphicContext::get()->device());

    auto vert_code = read_file_binary(FileSystem::get_shaders_directory() + "/triangle.vert.glsl.spv");
    auto frag_code = read_file_binary(FileSystem::get_shaders_directory() + "/triangle.frag.glsl.spv");

    ShaderDesc shader_descs[ 2 ];
    shader_descs[ 0 ].stage         = ShaderStage::eVertex;
    shader_descs[ 0 ].bytecode_size = vert_code.size() * sizeof(vert_code[ 0 ]);
    shader_descs[ 0 ].bytecode      = vert_code.data();
    shader_descs[ 1 ].stage         = ShaderStage::eFragment;
    shader_descs[ 1 ].bytecode_size = frag_code.size() * sizeof(frag_code[ 0 ]);
    shader_descs[ 1 ].bytecode      = frag_code.data();

    Shader* shaders[ 2 ] = {};
    create_shader(GraphicContext::get()->device(), &shader_descs[ 0 ], &shaders[ 0 ]);
    create_shader(GraphicContext::get()->device(), &shader_descs[ 1 ], &shaders[ 1 ]);

    create_descriptor_set_layout(GraphicContext::get()->device(), 2, shaders, &dsl);

    PipelineDesc  pipeline_desc{};
    VertexLayout& vertex_layout                 = pipeline_desc.vertex_layout;
    vertex_layout.binding_desc_count            = 1;
    vertex_layout.binding_descs[ 0 ].binding    = 0;
    vertex_layout.binding_descs[ 0 ].input_rate = VertexInputRate::eVertex;
    vertex_layout.binding_descs[ 0 ].stride     = 3 * sizeof(float);
    vertex_layout.attribute_desc_count          = 1;
    vertex_layout.attribute_descs[ 0 ].binding  = 0;
    vertex_layout.attribute_descs[ 0 ].format   = Format::eR32G32B32Sfloat;
    vertex_layout.attribute_descs[ 0 ].location = 0;
    vertex_layout.attribute_descs[ 0 ].offset   = 0;
    pipeline_desc.shader_count                  = 2;
    pipeline_desc.shaders[ 0 ]                  = shaders[ 0 ];
    pipeline_desc.shaders[ 1 ]                  = shaders[ 1 ];
    pipeline_desc.rasterizer_desc.cull_mode     = CullMode::eNone;
    pipeline_desc.rasterizer_desc.front_face    = FrontFace::eCounterClockwise;
    pipeline_desc.depth_state_desc.depth_test   = false;
    pipeline_desc.depth_state_desc.depth_write  = false;
    pipeline_desc.descriptor_set_layout         = dsl;
    pipeline_desc.render_pass                   = GraphicContext::get()->swapchain()->render_passes[ 0 ];

    create_graphics_pipeline(GraphicContext::get()->device(), &pipeline_desc, &pipeline);

    for (u32 i = 0; i < 2; ++i)
    {
        destroy_shader(GraphicContext::get()->device(), shaders[ i ]);
    }

    BufferDesc buffer_desc{};
    buffer_desc.size            = sizeof(vertices);
    buffer_desc.descriptor_type = DescriptorType::eVertexBuffer;
    buffer_desc.memory_usage    = MemoryUsage::eGpuOnly;
    create_buffer(GraphicContext::get()->device(), &buffer_desc, &vertex_buffer);

    ResourceLoader::upload_buffer(vertex_buffer, 0, sizeof(vertices), vertices);
}

void on_resize(u32 width, u32 height)
{
    GraphicContext::get()->on_resize(width, height);
}

void on_update(f32 delta_time)
{
    auto* context = GraphicContext::get();
    auto* cmd     = context->acquire_cmd();
    context->begin_frame();
    context->begin_render_pass(0.4, 0.1, 0.2, 1.0);
    cmd_bind_pipeline(cmd, pipeline);
    cmd_set_viewport(cmd, 0, 0, context->width(), context->height(), 0, 1.0f);
    cmd_set_scissor(cmd, 0, 0, context->width(), context->height());
    cmd_bind_vertex_buffer(cmd, vertex_buffer);
    cmd_draw(cmd, 3, 1, 0, 0);
    context->end_render_pass();
    context->end_frame();
}

void on_shutdown()
{
    auto* device = GraphicContext::get()->device();
    device_wait_idle(device);
    destroy_buffer(device, vertex_buffer);
    destroy_pipeline(device, pipeline);
    destroy_descriptor_set_layout(device, dsl);
    ResourceLoader::shutdown();
    GraphicContext::shutdown();
}

int main(int argc, char** argv)
{
    ApplicationConfig config;
    config.argc        = argc;
    config.argv        = argv;
    config.title       = "Sandbox";
    config.x           = 0;
    config.y           = 0;
    config.width       = SCREEN_WIDTH;
    config.height      = SCREEN_HEIGHT;
    config.log_level   = LogLevel::eTrace;
    config.on_init     = on_init;
    config.on_update   = on_update;
    config.on_resize   = on_resize;
    config.on_shutdown = on_shutdown;

    app_init(&config);
    app_run();
    app_shutdown();

    return 0;
}
