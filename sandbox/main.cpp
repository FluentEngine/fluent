#include <iostream>

#include "fluent/fluent.hpp"
#include "model.hpp"

using namespace fluent;

static constexpr u32 SCREEN_WIDTH  = 1400;
static constexpr u32 SCREEN_HEIGHT = 900;

static constexpr u32 MAX_COMMANDS         = 100;
static constexpr u32 MAX_OBJECTS          = 10000;
static constexpr u32 GEOMETRY_BUFFER_SIZE = 500 * 1024 * 1024;

struct GlobalUbo
{
    Matrix4 projection;
    Matrix4 view;
} global_ubo;

struct Geometry
{
    i32 first_vertex = 0;
    u32 first_index  = 0;
    u32 index_count  = 0;
};

struct GpuObjectData
{
    Matrix4 model;
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
};

struct DefaultResources
{
    Sampler* sampler = nullptr;
    Image*   magenta = nullptr;
} default_resources;

struct DrawData
{
    Geometry       geometry;
    Matrix4        transform;
    DescriptorSet* material_set;
};

DescriptorSetLayout* dsl;
Pipeline*            pipeline;

GeometryBuffer geometry_buffer;
FrameData      frames[ GraphicContext::frame_count() ];

std::vector<Image*>   loaded_textures;
std::vector<DrawData> draws;

Camera           camera;
CameraController camera_controller;

void create_pipeline(VertexLayout& layout)
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

    PipelineDesc pipeline_desc{};
    pipeline_desc.vertex_layout                = layout;
    pipeline_desc.shader_count                 = 2;
    pipeline_desc.shaders[ 0 ]                 = shaders[ 0 ];
    pipeline_desc.shaders[ 1 ]                 = shaders[ 1 ];
    pipeline_desc.rasterizer_desc.cull_mode    = CullMode::eBack;
    pipeline_desc.rasterizer_desc.front_face   = FrontFace::eCounterClockwise;
    pipeline_desc.depth_state_desc.depth_test  = true;
    pipeline_desc.depth_state_desc.depth_write = true;
    pipeline_desc.depth_state_desc.compare_op  = CompareOp::eLess;
    pipeline_desc.descriptor_set_layout        = dsl;
    pipeline_desc.render_pass                  = GraphicContext::get()->swapchain()->render_passes[ 0 ];

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
}

void destroy_geometry_buffer()
{
    Device* device = GraphicContext::get()->device();
    destroy_buffer(device, geometry_buffer.vertex_buffer);
    destroy_buffer(device, geometry_buffer.index_buffer);
}

void create_color_texture(Image** image, const Vector4& color)
{
    Device* device = GraphicContext::get()->device();

    ImageDesc image_desc{};
    image_desc.width           = 2;
    image_desc.height          = 2;
    image_desc.depth           = 1;
    image_desc.descriptor_type = DescriptorType::eSampledImage;
    image_desc.sample_count    = SampleCount::e1;
    image_desc.format          = Format::eR8G8B8A8Unorm;
    image_desc.mip_levels      = 1;
    image_desc.layer_count     = 1;

    create_image(device, &image_desc, image);

    struct Color
    {
        u8 r, g, b, a;
    };

    std::vector<Color> image_data(
        image_desc.width * image_desc.height,
        { ( u8 ) (color.r * 255.0f), ( u8 ) (color.g * 255.0f), ( u8 ) (color.b * 255.0f), ( u8 ) (color.a * 255.0f) });
    ResourceLoader::upload_image(*image, image_data.size() * sizeof(image_data[ 0 ]), image_data.data());
}

void create_default_resources()
{
    Device* device = GraphicContext::get()->device();

    SamplerDesc sampler_desc{};
    sampler_desc.mipmap_mode = SamplerMipmapMode::eLinear;
    sampler_desc.min_lod     = 0;
    sampler_desc.max_lod     = 16;
    create_sampler(device, &sampler_desc, &default_resources.sampler);
    create_color_texture(&default_resources.magenta, Vector4(1.0, 0, 1.0, 1.0));
}

void destroy_default_resources()
{
    Device* device = GraphicContext::get()->device();

    destroy_image(device, default_resources.magenta);
    destroy_sampler(device, default_resources.sampler);
}

Image* upload_mesh_texture(const Model& model, const Mesh& mesh, TextureType type)
{
    const Device* device = GraphicContext::get()->device();

    if (mesh.has_texture(type))
    {
        auto& texture = model.textures[ mesh.get_texture_index(type) ];
        create_image(device, &texture.desc, &loaded_textures.emplace_back());
        ResourceLoader::upload_image(loaded_textures.back(), texture.size, texture.data);
        return loaded_textures.back();
    }
    else
    {
        return default_resources.magenta;
    }
}

void upload_model(const Model& model, const Matrix4& transform)
{
    Device* device = GraphicContext::get()->device();

    for (auto& mesh : model.meshes)
    {
        auto& draw = draws.emplace_back();

        u64 vertex_upload_size = mesh.vertex_count * model.vertex_stride * sizeof(model.vertices[ 0 ]);
        ResourceLoader::upload_buffer(
            geometry_buffer.vertex_buffer, geometry_buffer.current_vertex_offset, vertex_upload_size,
            model.vertices.data());

        u64 index_upload_size = mesh.index_count * sizeof(model.indices[ 0 ]);
        ResourceLoader::upload_buffer(
            geometry_buffer.index_buffer, geometry_buffer.current_index_offset, index_upload_size,
            model.indices.data());

        auto& geometry        = draw.geometry;
        geometry.first_index  = geometry_buffer.total_index_count;
        geometry.first_vertex = geometry_buffer.total_vertex_count;
        geometry.index_count  = mesh.index_count;

        geometry_buffer.total_index_count += mesh.index_count;
        geometry_buffer.total_vertex_count += mesh.vertex_count;

        geometry_buffer.current_vertex_offset += vertex_upload_size;
        geometry_buffer.current_index_offset += index_upload_size;

        draw.transform = transform;

        // Upload mesh textures
        u32 first_texture = loaded_textures.size();

        ImageDescriptor image_descriptors[ static_cast<u32>(TextureType::eCount) ] = {};

        for (u32 i = 0; i < static_cast<u32>(TextureType::eCount); i++)
        {
            image_descriptors[ i ].resource_state = ResourceState::eShaderReadOnly;
            image_descriptors[ i ].image          = upload_mesh_texture(model, mesh, static_cast<TextureType>(i));
        }

        // create material set
        DescriptorSetDesc desc{};
        desc.descriptor_set_layout = dsl;
        desc.set                   = 1;
        create_descriptor_set(GraphicContext::get()->device(), &desc, &draw.material_set);

        ImageDescriptor sampler_descriptor = {};
        sampler_descriptor.sampler         = default_resources.sampler;

        DescriptorWrite writes[ 2 ]   = {};
        writes[ 0 ].binding           = 0;
        writes[ 0 ].descriptor_count  = 1;
        writes[ 0 ].descriptor_type   = DescriptorType::eSampler;
        writes[ 0 ].image_descriptors = &sampler_descriptor;
        writes[ 1 ].binding           = 1;
        writes[ 1 ].descriptor_count  = static_cast<u32>(TextureType::eCount);
        writes[ 1 ].descriptor_type   = DescriptorType::eSampledImage;
        writes[ 1 ].image_descriptors = image_descriptors;

        update_descriptor_set(GraphicContext::get()->device(), draw.material_set, 2, writes);
    }
}

void unload_models()
{
    Device* device = GraphicContext::get()->device();

    for (u32 i = 0; i < loaded_textures.size(); ++i)
    {
        destroy_image(device, loaded_textures[ i ]);
    }

    for (u32 i = 0; i < draws.size(); ++i)
    {
        destroy_descriptor_set(device, draws[ i ].material_set);
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

    VertexComponents components =
        (VertexComponents::ePosition | VertexComponents::eNormal | VertexComponents::eTexcoord |
         VertexComponents::eTangent);

    // temporary solution, all models will be load with same layout
    VertexLayout layout;
    fill_vertex_layout(layout, components);
    create_pipeline(layout);
    create_frames();
    create_geometry_buffer();
    create_default_resources();

    Model model;
    load_model(model, components, FileSystem::get_models_directory() + "damaged-helmet/DamagedHelmet.gltf");
    upload_model(model, translate(Matrix4(1.0), Vector3(0.0, 1.0, 0.0)));
    model = {};
    load_model(model, components, FileSystem::get_models_directory() + "plane.gltf");
    upload_model(model, Matrix4(1.0));
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
    context->begin_render_pass(0.52, 0.8, 0.9, 1.0);
    cmd_bind_pipeline(cmd, pipeline);
    cmd_bind_descriptor_set(cmd, 0, frame.const_set, pipeline);
    cmd_set_viewport(cmd, 0, 0, context->width(), context->height(), 0.1f, 1.0f);
    cmd_set_scissor(cmd, 0, 0, context->width(), context->height());
    cmd_bind_vertex_buffer(cmd, geometry_buffer.vertex_buffer, 0);
    cmd_bind_index_buffer_u32(cmd, geometry_buffer.index_buffer, 0);

    auto* objects_data = ( GpuObjectData* ) ResourceLoader::begin_upload_buffer(frame.objects_buffer);

    for (u32 i = 0; i < draws.size(); ++i)
    {
        DrawData& draw = draws[ i ];

        cmd_bind_descriptor_set(cmd, 1, draw.material_set, pipeline);

        objects_data[ i ].model = draw.transform;

        i32 first_vertex = draw.geometry.first_vertex;
        u32 first_index  = draw.geometry.first_index;
        u32 index_count  = draw.geometry.index_count;

        cmd_draw_indexed(cmd, index_count, 1, first_index, first_vertex, i);
    }

    ResourceLoader::end_upload_buffer(frame.objects_buffer);

    context->end_render_pass();
    context->end_frame();
}

void on_shutdown()
{
    auto* device = GraphicContext::get()->device();
    device_wait_idle(device);
    unload_models();
    destroy_default_resources();
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
