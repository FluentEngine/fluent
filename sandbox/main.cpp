#include <iostream>

#include "fluent/fluent.hpp"
#include "model.hpp"

using namespace fluent;

static constexpr u32 SCREEN_WIDTH  = 1400;
static constexpr u32 SCREEN_HEIGHT = 900;

static constexpr b32 DRAW_INDIRECT = false;

static constexpr u32 MAX_COMMANDS         = 100;
static constexpr u32 MAX_OBJECTS          = 10000;
static constexpr u32 GEOMETRY_BUFFER_SIZE = 500 * 1024 * 1024;

struct GlobalUbo
{
    Matrix4 projection;
    Matrix4 view;
} global_ubo;

struct Material
{
    u32 base_color = 0;
    u32 normal     = 0;
};

struct LoadedTexture
{
    u32       id   = 0;
    ImageDesc desc = {};
    u64       size = 0;
    void*     data = nullptr;
};

struct Geometry
{
    i32 first_vertex = 0;
    u32 first_index  = 0;
    u32 index_count  = 0;
};

struct GpuObjectData
{
    Matrix4 model;
    Vector4 color;
};

struct GeometryBuffer
{
    u64     current_vertex_offset = 0;
    u32     total_vertex_count    = 0;
    Buffer* vertex_buffer         = nullptr;
    u64     current_index_offset  = 0;
    u32     total_index_count     = 0;
    Buffer* index_buffer          = nullptr;
    Buffer* indirect_buffer       = nullptr;
};

struct FrameData
{
    Buffer*        global_buffer  = nullptr;
    Buffer*        objects_buffer = nullptr;
    DescriptorSet* const_set      = nullptr;
    DescriptorSet* material_set   = nullptr;
};

struct DefaultResources
{
    Sampler* sampler = nullptr;
    Image*   magenta = nullptr;
} default_resources;

DescriptorSetLayout* dsl;
Pipeline*            pipeline;

GeometryBuffer geometry_buffer;
FrameData      frames[ GraphicContext::frame_count() ];

std::vector<Geometry> geometries;
std::vector<Vector4>  colors;
std::vector<Matrix4>  transforms;

Camera           camera;
CameraController camera_controller;

void create_pipeline()
{
    auto vert_code = read_file_binary(FileSystem::get_shaders_directory() + "/main.vert.glsl.spv");
    auto frag_code = read_file_binary(FileSystem::get_shaders_directory() + "/main.frag.glsl.spv");

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
    pipeline_desc.rasterizer_desc.cull_mode     = CullMode::eBack;
    pipeline_desc.rasterizer_desc.front_face    = FrontFace::eCounterClockwise;
    pipeline_desc.depth_state_desc.depth_test   = true;
    pipeline_desc.depth_state_desc.depth_write  = true;
    pipeline_desc.depth_state_desc.compare_op   = CompareOp::eLess;
    pipeline_desc.descriptor_set_layout         = dsl;
    pipeline_desc.render_pass                   = GraphicContext::get()->swapchain()->render_passes[ 0 ];

    create_graphics_pipeline(GraphicContext::get()->device(), &pipeline_desc, &pipeline);

    for (u32 i = 0; i < 2; ++i)
    {
        destroy_shader(GraphicContext::get()->device(), shaders[ i ]);
    }
}

void destroy_pipeline()
{
    Device* device = GraphicContext::get()->device();
    destroy_pipeline(device, pipeline);
    destroy_descriptor_set_layout(device, dsl);
}

void create_frames()
{
    for (u32 i = 0; i < GraphicContext::frame_count(); ++i)
    {
        // Create buffers

        BufferDesc global_ubo_buffer_desc{};
        global_ubo_buffer_desc.descriptor_type = DescriptorType::eUniformBuffer;
        global_ubo_buffer_desc.memory_usage    = MemoryUsage::eCpuToGpu;
        global_ubo_buffer_desc.size            = sizeof(GlobalUbo);
        create_buffer(GraphicContext::get()->device(), &global_ubo_buffer_desc, &frames[ i ].global_buffer);

        global_ubo.projection = camera.get_projection_matrix();
        global_ubo.view       = camera.get_view_matrix();

        ResourceLoader::upload_buffer(frames[ i ].global_buffer, 0, sizeof(GlobalUbo), &global_ubo);

        BufferDesc gpu_objects_data_buffer_desc{};
        gpu_objects_data_buffer_desc.descriptor_type = DescriptorType::eStorageBuffer;
        gpu_objects_data_buffer_desc.size            = MAX_OBJECTS * sizeof(GpuObjectData);
        gpu_objects_data_buffer_desc.memory_usage    = MemoryUsage::eCpuToGpu;

        create_buffer(GraphicContext::get()->device(), &gpu_objects_data_buffer_desc, &frames[ i ].objects_buffer);

        // Create descriptors

        DescriptorSetDesc desc{};
        desc.descriptor_set_layout = dsl;
        desc.set                   = 0;

        create_descriptor_set(GraphicContext::get()->device(), &desc, &frames[ i ].const_set);
        desc.set = 1;
        create_descriptor_set(GraphicContext::get()->device(), &desc, &frames[ i ].material_set);

        BufferDescriptor buffer_descriptors[ 2 ] = {};
        buffer_descriptors[ 0 ].buffer           = frames[ i ].global_buffer;
        buffer_descriptors[ 0 ].offset           = 0;
        buffer_descriptors[ 0 ].range            = sizeof(GlobalUbo);
        buffer_descriptors[ 1 ].buffer           = frames[ i ].objects_buffer;
        buffer_descriptors[ 1 ].offset           = 0;
        buffer_descriptors[ 1 ].range            = sizeof(GpuObjectData) * MAX_OBJECTS;

        DescriptorWrite writes[ 2 ]    = {};
        writes[ 0 ].binding            = 0;
        writes[ 0 ].descriptor_type    = DescriptorType::eUniformBuffer;
        writes[ 0 ].descriptor_count   = 1;
        writes[ 0 ].buffer_descriptors = &buffer_descriptors[ 0 ];
        writes[ 1 ].binding            = 1;
        writes[ 1 ].descriptor_type    = DescriptorType::eStorageBuffer;
        writes[ 1 ].descriptor_count   = 1;
        writes[ 1 ].buffer_descriptors = &buffer_descriptors[ 1 ];

        update_descriptor_set(GraphicContext::get()->device(), frames[ i ].const_set, 2, writes);
    }
}

void destroy_frames()
{
    Device* device = GraphicContext::get()->device();
    for (u32 i = 0; i < GraphicContext::frame_count(); ++i)
    {
        destroy_descriptor_set(device, frames[ i ].material_set);
        destroy_descriptor_set(device, frames[ i ].const_set);
        destroy_buffer(device, frames[ i ].global_buffer);
        destroy_buffer(device, frames[ i ].objects_buffer);
    }
}

void create_geometry_buffer()
{
    BufferDesc buffer_desc{};
    buffer_desc.size            = GEOMETRY_BUFFER_SIZE;
    buffer_desc.descriptor_type = DescriptorType::eVertexBuffer;
    buffer_desc.memory_usage    = MemoryUsage::eGpuOnly;
    create_buffer(GraphicContext::get()->device(), &buffer_desc, &geometry_buffer.vertex_buffer);
    buffer_desc.descriptor_type = DescriptorType::eIndexBuffer;
    create_buffer(GraphicContext::get()->device(), &buffer_desc, &geometry_buffer.index_buffer);

    if constexpr (DRAW_INDIRECT)
    {
        buffer_desc.size            = MAX_COMMANDS * sizeof(DrawIndexedIndirectCommand);
        buffer_desc.descriptor_type = DescriptorType::eIndirectBuffer | DescriptorType::eStorageBuffer;
        buffer_desc.memory_usage    = MemoryUsage::eCpuToGpu;
        create_buffer(GraphicContext::get()->device(), &buffer_desc, &geometry_buffer.indirect_buffer);
    }
}

void destroy_geometry_buffer()
{
    Device* device = GraphicContext::get()->device();
    destroy_buffer(device, geometry_buffer.vertex_buffer);
    destroy_buffer(device, geometry_buffer.index_buffer);
    if constexpr (DRAW_INDIRECT)
    {
        destroy_buffer(device, geometry_buffer.indirect_buffer);
    }
}

void create_default_resources()
{
    Device* device = GraphicContext::get()->device();

    SamplerDesc sampler_desc{};
    create_sampler(device, &sampler_desc, &default_resources.sampler);
    ImageDesc image_desc{};
    image_desc.width           = 2;
    image_desc.height          = 2;
    image_desc.depth           = 1;
    image_desc.descriptor_type = DescriptorType::eSampledImage;
    image_desc.sample_count    = SampleCount::e1;
    image_desc.format          = Format::eR8G8B8A8Unorm;
    image_desc.mip_levels      = 1;
    image_desc.layer_count     = 1;

    create_image(device, &image_desc, &default_resources.magenta);

    struct Color
    {
        u8 r, g, b, a;
    };

    std::vector<Color> image_data(image_desc.width * image_desc.height, { 255, 0, 255, 255 });
    ResourceLoader::upload_image(
        default_resources.magenta, image_data.size() * sizeof(image_data[ 0 ]), image_data.data());
}

void destroy_default_resources()
{
    Device* device = GraphicContext::get()->device();

    destroy_image(device, default_resources.magenta);
    destroy_sampler(device, default_resources.sampler);
}

void upload_model(const Model& model)
{
    for (auto& mesh : model.meshes)
    {
        u64 vertex_upload_size = mesh.vertex_count * sizeof(model.vertices[ 0 ]);
        ResourceLoader::upload_buffer(
            geometry_buffer.vertex_buffer, geometry_buffer.current_vertex_offset, vertex_upload_size,
            model.vertices.data());

        u64 index_upload_size = mesh.index_count * sizeof(model.indices[ 0 ]);
        ResourceLoader::upload_buffer(
            geometry_buffer.index_buffer, geometry_buffer.current_index_offset, index_upload_size,
            model.indices.data());

        auto& geometry        = geometries.emplace_back();
        geometry.first_index  = geometry_buffer.total_index_count;
        geometry.first_vertex = geometry_buffer.total_vertex_count;
        geometry.index_count  = mesh.index_count;

        geometry_buffer.total_index_count += mesh.index_count;
        geometry_buffer.total_vertex_count += mesh.vertex_count / mesh.vertex_stride;

        geometry_buffer.current_vertex_offset += vertex_upload_size;
        geometry_buffer.current_index_offset += index_upload_size;
    }
}

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

    camera.init_camera(Vector3(0.0f, 1.0, 3.0f), Vector3(0.0, 0.0, -1.0), Vector3(0.0, 1.0, 0.0));
    camera_controller.init(get_app_input_system(), camera);

    create_pipeline();
    create_frames();
    create_geometry_buffer();

    Model model = create_triangle();
    upload_model(model);
    upload_model(model);
    colors.emplace_back(Vector4(0.0, 1.0, 1.0, 1.0));
    colors.emplace_back(Vector4(1.0, 1.0, 0.0, 1.0));
    transforms.emplace_back(Matrix4(1.0));
    transforms.emplace_back(translate(Matrix4(1.0), Vector3(1.0, 0.0, 0.0)));
}

void on_resize(u32 width, u32 height)
{
    GraphicContext::get()->on_resize(width, height);
    camera.on_resize(width, height);
    global_ubo.projection = camera.get_projection_matrix();
}

void on_update(f32 delta_time)
{
    auto* context = GraphicContext::get();
    auto* cmd     = context->acquire_cmd();
    auto& frame   = frames[ context->frame_index() ];

    camera_controller.update(delta_time);
    global_ubo.view = camera.get_view_matrix();
    ResourceLoader::upload_buffer(frame.global_buffer, 0, sizeof(GlobalUbo), &global_ubo);

    context->begin_frame();
    context->begin_render_pass(0.4, 0.1, 0.2, 1.0);
    cmd_bind_pipeline(cmd, pipeline);
    cmd_bind_descriptor_set(cmd, 0, frame.const_set, pipeline);
    cmd_bind_descriptor_set(cmd, 1, frame.material_set, pipeline);
    cmd_set_viewport(cmd, 0, 0, context->width(), context->height(), 0.1f, 1.0f);
    cmd_set_scissor(cmd, 0, 0, context->width(), context->height());
    cmd_bind_vertex_buffer(cmd, geometry_buffer.vertex_buffer, 0);
    cmd_bind_index_buffer_u32(cmd, geometry_buffer.index_buffer, 0);

    if constexpr (DRAW_INDIRECT)
    {
        auto* draw_commands =
            ( DrawIndexedIndirectCommand* ) ResourceLoader::begin_upload_buffer(geometry_buffer.indirect_buffer);
        auto* objects_data = ( GpuObjectData* ) ResourceLoader::begin_upload_buffer(frame.objects_buffer);

        for (u32 i = 0; i < geometries.size(); i++)
        {
            objects_data[ i ].model = transforms[ i ];
            objects_data[ i ].color = colors[ i ];

            Geometry& geometry               = geometries[ i ];
            draw_commands[ i ].indexCount    = geometries[ i ].index_count;
            draw_commands[ i ].instanceCount = 1;
            draw_commands[ i ].firstIndex    = geometries[ i ].first_index;
            draw_commands[ i ].vertexOffset  = geometries[ i ].first_vertex;
            draw_commands[ i ].firstInstance = i;
        }

        ResourceLoader::end_upload_buffer(geometry_buffer.indirect_buffer);
        ResourceLoader::end_upload_buffer(frame.objects_buffer);

        u32 draw_stride = sizeof(DrawIndexedIndirectCommand);
        cmd_draw_indexed_indirect(cmd, geometry_buffer.indirect_buffer, 0, geometries.size(), draw_stride);
    }
    else
    {
        auto* objects_data = ( GpuObjectData* ) ResourceLoader::begin_upload_buffer(frame.objects_buffer);

        for (u32 i = 0; i < geometries.size(); ++i)
        {
            objects_data[ i ].model = transforms[ i ];
            objects_data[ i ].color = colors[ i ];

            i32 first_vertex = geometries[ i ].first_vertex;
            u32 first_index  = geometries[ i ].first_index;
            u32 index_count  = geometries[ i ].index_count;
            cmd_push_constants(cmd, pipeline, 0, sizeof(Vector4), &colors[ i ]);
            cmd_draw_indexed(cmd, index_count, 1, first_index, first_vertex, i);
        }

        ResourceLoader::end_upload_buffer(frame.objects_buffer);
    }

    context->end_render_pass();
    context->end_frame();
}

void on_shutdown()
{
    auto* device = GraphicContext::get()->device();
    device_wait_idle(device);
    destroy_geometry_buffer();
    destroy_frames();
    destroy_pipeline();
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
