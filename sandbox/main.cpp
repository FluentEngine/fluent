#include <iostream>

#include "fluent/fluent.hpp"
#include "cgltf.h"

using namespace fluent;

static constexpr u32 SCREEN_WIDTH  = 1400;
static constexpr u32 SCREEN_HEIGHT = 900;

static constexpr b32 DRAW_INDIRECT = false;

struct GlobalUbo
{
    Matrix4 projection;
    Matrix4 view;
} global_ubo;

struct Mesh
{
    std::vector<Vector3> positions;
    std::vector<Vector3> normals;
    std::vector<Vector2> texcoords;
    std::vector<u32>     indices;
};

struct Geometry
{
    i32 first_vertex = 0;
    u32 first_index  = 0;
    u32 index_count  = 0;
};

DescriptorSetLayout* dsl;
Pipeline*            pipeline;
DescriptorSet*       set;

static constexpr u32 MAX_COMMANDS = 100;

struct GeometryBuffer
{
    u64     current_vertex_offset = 0;
    u32     total_vertex_count    = 0;
    Buffer* vertex_buffer         = nullptr;
    u64     current_index_offset  = 0;
    u32     total_index_count     = 0;
    Buffer* index_buffer          = nullptr;
    Buffer* indirect_buffer       = nullptr;
} geometry_buffer;

Buffer*               global_ubo_buffer = nullptr;
std::vector<Geometry> geometries;
std::vector<Vector4>  colors;

Camera           camera;
CameraController camera_controller;

void gltf_read_float(
    const float* accessor_data, cgltf_size accessor_num_components, cgltf_size index, cgltf_float* out,
    cgltf_size out_element_size)
{
    const float* input = &accessor_data[ accessor_num_components * index ];

    for (cgltf_size ii = 0; ii < out_element_size; ++ii)
    {
        out[ ii ] = (ii < accessor_num_components) ? input[ ii ] : 0.0f;
    }
}

void process_mesh_node(Mesh& result, cgltf_node* node)
{
    cgltf_mesh* mesh = node->mesh;

    if (mesh)
    {
        Matrix4 node_to_world = Matrix4(1.0);
        cgltf_node_transform_world(node, &node_to_world[ 0 ][ 0 ]);

        for (cgltf_size primitive_index = 0; primitive_index < mesh->primitives_count; ++primitive_index)
        {
            cgltf_primitive& primitive    = mesh->primitives[ primitive_index ];
            cgltf_size       vertex_count = primitive.attributes[ 0 ].data->count;

            for (cgltf_size attribute_index = 0; attribute_index < primitive.attributes_count; ++attribute_index)
            {
                cgltf_attribute& attribute      = primitive.attributes[ attribute_index ];
                cgltf_accessor*  accessor       = attribute.data;
                cgltf_size       accessor_count = accessor->count;

                FT_ASSERT(vertex_count == accessor_count && "Invalid attribute count");

                cgltf_size         float_count = cgltf_accessor_unpack_floats(accessor, nullptr, 0);
                std::vector<float> accessor_data(float_count);
                cgltf_accessor_unpack_floats(accessor, accessor_data.data(), float_count);

                cgltf_size components_count = cgltf_num_components(accessor->type);

                if (attribute.type == cgltf_attribute_type_position && attribute_index == 0)
                {
                    result.positions.reserve(result.positions.size() + accessor_count);

                    Vector3 position;

                    for (cgltf_size v = 0; v < accessor_count; ++v)
                    {
                        gltf_read_float(accessor_data.data(), components_count, v, &position.x, 3);
                        position = node_to_world * Vector4(position, 1.0);
                        result.positions.emplace_back(position);
                    }
                }
                else if (attribute.type == cgltf_attribute_type_normal && attribute_index == 0)
                {
                    result.normals.reserve(result.normals.size() + accessor_count);

                    Vector3 normal;

                    for (cgltf_size v = 0; v < accessor_count; ++v)
                    {
                        gltf_read_float(accessor_data.data(), components_count, v, &normal.x, 3);
                        normal = node_to_world * Vector4(normal, 1.0);
                        result.normals.emplace_back(normal);
                    }
                }
                else if (attribute.type == cgltf_attribute_type_texcoord && attribute_index == 0)
                {
                    result.texcoords.reserve(result.texcoords.size() + accessor_count);

                    Vector2 texcoord;

                    for (cgltf_size v = 0; v < accessor_count; ++v)
                    {
                        gltf_read_float(accessor_data.data(), components_count, v, &texcoord.x, 2);
                        result.texcoords.emplace_back(texcoord);
                    }
                }
            }

            if (primitive.indices != NULL)
            {
                cgltf_accessor* accessor = primitive.indices;

                for (cgltf_size v = 0; v < accessor->count; v += 3)
                {
                    for (int i = 0; i < 3; ++i)
                    {
                        u32 index = u32(cgltf_accessor_read_index(accessor, v + i));
                        result.indices.push_back(index);
                    }
                }
            }
            else
            {
                // TODO: Count indices
                FT_ASSERT(false && "Not implemented");
            }
        }
    }

    for (cgltf_size child_index = 0; child_index < node->children_count; ++child_index)
        process_mesh_node(result, node->children[ child_index ]);
}

void load_mesh(Mesh& mesh, const std::string& filepath)
{
    cgltf_options options = {};
    cgltf_data*   data    = NULL;
    cgltf_result  result  = cgltf_parse_file(&options, filepath.c_str(), &data);

    FT_ASSERT(result == cgltf_result_success && "Model loading failed");
    result = cgltf_load_buffers(&options, data, filepath.c_str());
    FT_ASSERT(result == cgltf_result_success && "Model buffers loading failed");

    for (cgltf_size scene_index = 0; scene_index < data->scenes_count; ++scene_index)
    {
        cgltf_scene* scene = &data->scenes[ scene_index ];

        for (cgltf_size node_index = 0; node_index < scene->nodes_count; ++node_index)
        {
            cgltf_node* node = scene->nodes[ node_index ];
            process_mesh_node(mesh, node);
        }
    }

    cgltf_free(data);
}

std::vector<f32> process_geometry(const Mesh& mesh)
{
    std::vector<f32> result;
    result.reserve(mesh.positions.size() * 3);

    for (u32 i = 0; i < mesh.positions.size(); i++)
    {
        result.emplace_back(mesh.positions[ i ].x);
        result.emplace_back(mesh.positions[ i ].y);
        result.emplace_back(mesh.positions[ i ].z);
    }

    return result;
}

void load_geometry(Geometry& geometry, const Mesh& mesh)
{
    auto  vertices = process_geometry(mesh);
    auto& indices  = mesh.indices;

    u64 vertex_upload_size = vertices.size() * sizeof(vertices[ 0 ]);
    ResourceLoader::upload_buffer(
        geometry_buffer.vertex_buffer, geometry_buffer.current_vertex_offset, vertex_upload_size, vertices.data());
    u64 index_upload_size = indices.size() * sizeof(indices[ 0 ]);
    ResourceLoader::upload_buffer(
        geometry_buffer.index_buffer, geometry_buffer.current_index_offset, index_upload_size, indices.data());

    geometry.first_index  = geometry_buffer.total_index_count;
    geometry.first_vertex = geometry_buffer.total_vertex_count;
    geometry.index_count  = indices.size();

    geometry_buffer.total_index_count += indices.size();
    geometry_buffer.total_vertex_count += mesh.positions.size();

    geometry_buffer.current_vertex_offset += vertex_upload_size;
    geometry_buffer.current_index_offset += index_upload_size;
}

void create_descriptor_set()
{
    DescriptorSetDesc desc{};
    desc.descriptor_set_layout = dsl;
    desc.set                   = 0;

    create_descriptor_set(GraphicContext::get()->device(), &desc, &set);

    BufferDescriptor buffer_descriptor{};
    buffer_descriptor.buffer = global_ubo_buffer;
    buffer_descriptor.offset = 0;
    buffer_descriptor.range  = sizeof(GlobalUbo);

    DescriptorWrite write{};
    write.binding            = 0;
    write.descriptor_type    = DescriptorType::eUniformBuffer;
    write.descriptor_count   = 1;
    write.buffer_descriptors = &buffer_descriptor;

    update_descriptor_set(GraphicContext::get()->device(), set, 1, &write);
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

    BufferDesc global_ubo_buffer_desc{};
    global_ubo_buffer_desc.descriptor_type = DescriptorType::eUniformBuffer;
    global_ubo_buffer_desc.memory_usage    = MemoryUsage::eCpuToGpu;
    global_ubo_buffer_desc.size            = sizeof(GlobalUbo);
    create_buffer(GraphicContext::get()->device(), &global_ubo_buffer_desc, &global_ubo_buffer);

    global_ubo.projection = camera.get_projection_matrix();
    global_ubo.view       = camera.get_view_matrix();

    ResourceLoader::upload_buffer(global_ubo_buffer, 0, sizeof(GlobalUbo), &global_ubo);

    ::create_descriptor_set();

    BufferDesc buffer_desc{};
    buffer_desc.size            = 500 * 1024 * 1024;
    buffer_desc.descriptor_type = DescriptorType::eVertexBuffer;
    buffer_desc.memory_usage    = MemoryUsage::eGpuOnly;
    create_buffer(GraphicContext::get()->device(), &buffer_desc, &geometry_buffer.vertex_buffer);
    buffer_desc.descriptor_type = DescriptorType::eIndexBuffer;
    create_buffer(GraphicContext::get()->device(), &buffer_desc, &geometry_buffer.index_buffer);

    if constexpr (DRAW_INDIRECT)
    {
        buffer_desc.size            = MAX_COMMANDS * sizeof(VkDrawIndexedIndirectCommand);
        buffer_desc.descriptor_type = DescriptorType::eIndirectBuffer | DescriptorType::eStorageBuffer;
        buffer_desc.memory_usage    = MemoryUsage::eCpuToGpu;
        create_buffer(GraphicContext::get()->device(), &buffer_desc, &geometry_buffer.indirect_buffer);
    }

    auto generate_color = []() {
        Vector4 color;
        color.r = ( f32 ) (rand() % 255) / 255.0f;
        color.g = ( f32 ) (rand() % 255) / 255.0f;
        color.b = ( f32 ) (rand() % 255) / 255.0f;
        color.a = 1.0f;

        return color;
    };

    std::vector<Vector3> vertices{ Vector3(-0.5, -0.5, 0.0), Vector3(0.5, -0.5, 0.0), Vector3(0.0, 0.5, 0.0) };

    Mesh mesh{};
    load_mesh(mesh, FileSystem::get_models_directory() + "plane.gltf");
    load_geometry(geometries.emplace_back(), mesh);
    mesh = {};
    load_mesh(mesh, FileSystem::get_models_directory() + "venus.gltf");
    load_geometry(geometries.emplace_back(), mesh);

    colors.emplace_back(generate_color());
    colors.emplace_back(generate_color());
}

void on_resize(u32 width, u32 height)
{
    GraphicContext::get()->on_resize(width, height);
    camera.on_resize(width, height);
    global_ubo.projection = camera.get_projection_matrix();
}

void on_update(f32 delta_time)
{
    camera_controller.update(delta_time);
    global_ubo.view = camera.get_view_matrix();
    ResourceLoader::upload_buffer(global_ubo_buffer, 0, sizeof(GlobalUbo), &global_ubo);

    auto* context = GraphicContext::get();
    auto* cmd     = context->acquire_cmd();
    context->begin_frame();
    context->begin_render_pass(0.4, 0.1, 0.2, 1.0);
    cmd_bind_pipeline(cmd, pipeline);
    cmd_bind_descriptor_set(cmd, 0, set, pipeline);
    cmd_set_viewport(cmd, 0, 0, context->width(), context->height(), 0.1f, 1.0f);
    cmd_set_scissor(cmd, 0, 0, context->width(), context->height());
    cmd_bind_vertex_buffer(cmd, geometry_buffer.vertex_buffer, 0);
    cmd_bind_index_buffer_u32(cmd, geometry_buffer.index_buffer, 0);

    if constexpr (DRAW_INDIRECT)
    {
        map_memory(context->device(), geometry_buffer.indirect_buffer);
        VkDrawIndexedIndirectCommand* draw_commands =
            ( VkDrawIndexedIndirectCommand* ) geometry_buffer.indirect_buffer->mapped_memory;

        cmd_push_constants(cmd, pipeline, 0, sizeof(Vector4), &colors[ 0 ]);

        for (u32 i = 0; i < geometries.size(); i++)
        {
            Geometry& geometry               = geometries[ i ];
            draw_commands[ i ].indexCount    = geometries[ i ].index_count;
            draw_commands[ i ].instanceCount = 1;
            draw_commands[ i ].firstIndex    = geometries[ i ].first_index;
            draw_commands[ i ].vertexOffset  = geometries[ i ].first_vertex;
            draw_commands[ i ].firstInstance = 0;
        }

        u32 draw_stride = sizeof(VkDrawIndexedIndirectCommand);
        vkCmdDrawIndexedIndirect(
            cmd->command_buffer, geometry_buffer.indirect_buffer->buffer, 0, geometries.size(), draw_stride);

        unmap_memory(context->device(), geometry_buffer.indirect_buffer);
    }
    else
    {
        for (u32 i = 0; i < geometries.size(); ++i)
        {
            i32 first_vertex = geometries[ i ].first_vertex;
            u32 first_index  = geometries[ i ].first_index;
            u32 index_count  = geometries[ i ].index_count;
            cmd_push_constants(cmd, pipeline, 0, sizeof(Vector4), &colors[ i ]);
            cmd_draw_indexed(cmd, index_count, 1, first_index, first_vertex, 0);
        }
    }

    context->end_render_pass();
    context->end_frame();
}

void on_shutdown()
{
    auto* device = GraphicContext::get()->device();
    device_wait_idle(device);
    destroy_buffer(device, global_ubo_buffer);
    destroy_buffer(device, geometry_buffer.vertex_buffer);
    destroy_buffer(device, geometry_buffer.index_buffer);
    if constexpr (DRAW_INDIRECT)
    {
        destroy_buffer(device, geometry_buffer.indirect_buffer);
    }
    destroy_pipeline(device, pipeline);
    destroy_descriptor_set(device, set);
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
