#include <iostream>

#include "fluent/fluent.hpp"

using namespace fluent;

static constexpr u32 FRAME_COUNT = 2;
u32                  frame_index = 0;

struct CameraUBO
{
    Matrix4 projection;
    Matrix4 view;
};

struct PushConstantBlock
{
    Matrix4 model;
    Vector4 viewPosition  = Vector4(0.0, 0.0, 2.0, 0.0);
    Vector4 lightPosition = Vector4(1.0, 2.0, 2.0, 0.0);
};

struct TextureIndices
{
    i32 diffuse = -1;
    i32 normal  = -1;
};

Renderer    renderer;
Device      device;
Queue       queue;
CommandPool command_pool;
Semaphore   image_available_semaphores[ FRAME_COUNT ];
Semaphore   rendering_finished_semaphores[ FRAME_COUNT ];
Fence       in_flight_fences[ FRAME_COUNT ];
b32         command_buffers_recorded[ FRAME_COUNT ];

Swapchain     swapchain;
CommandBuffer command_buffers[ FRAME_COUNT ];

DescriptorSetLayout descriptor_set_layout;
Pipeline            pipeline;

Buffer  uniform_buffer;
Sampler sampler;

DescriptorSet  descriptor_set;
Model          model;
TextureIndices texture_indices;

CameraUBO camera_ubo;

void on_init()
{
    app_set_shaders_directory("../../../examples/shaders/04_model_loading");
    app_set_textures_directory("../../../examples/textures");
    app_set_models_directory("../../../examples/models");

    RendererDesc renderer_desc{};
    renderer_desc.vulkan_allocator = nullptr;
    renderer                       = create_renderer(renderer_desc);

    DeviceDesc device_desc{};
    device_desc.frame_in_use_count = 2;
    create_device(renderer, device_desc, device);

    QueueDesc queue_desc{};
    queue_desc.queue_type = QueueType::eGraphics;
    get_queue(device, queue_desc, queue);

    CommandPoolDesc command_pool_desc{};
    command_pool_desc.queue = &queue;
    command_pool            = create_command_pool(device, command_pool_desc);

    allocate_command_buffers(device, command_pool, FRAME_COUNT, command_buffers);

    for (u32 i = 0; i < FRAME_COUNT; ++i)
    {
        image_available_semaphores[ i ]    = create_semaphore(device);
        rendering_finished_semaphores[ i ] = create_semaphore(device);
        in_flight_fences[ i ]              = create_fence(device);
        command_buffers_recorded[ i ]      = false;
    }

    SwapchainDesc swapchain_desc{};
    swapchain_desc.width           = window_get_width(get_app_window());
    swapchain_desc.height          = window_get_height(get_app_window());
    swapchain_desc.queue           = &queue;
    swapchain_desc.min_image_count = FRAME_COUNT;
    swapchain_desc.builtin_depth   = true;

    swapchain = create_swapchain(device, swapchain_desc);

    Shader shaders[ 2 ];
    shaders[ 0 ] = load_shader(device, "main.vert");
    shaders[ 1 ] = load_shader(device, "main.frag");

    descriptor_set_layout = create_descriptor_set_layout(device, 2, shaders);

    LoadModelDescription load_model_desc{};
    load_model_desc.filename        = "backpack/backpack.obj";
    load_model_desc.load_bitangents = true;
    load_model_desc.load_normals    = true;
    load_model_desc.load_tangents   = true;
    load_model_desc.load_tex_coords = true;
    load_model_desc.flip_uvs        = true;

    ModelLoader model_loader;
    model = model_loader.load(&device, load_model_desc);

    PipelineDesc pipeline_desc{};
    model_loader.get_vertex_binding_description(pipeline_desc.binding_desc_count, pipeline_desc.binding_descs);
    model_loader.get_vertex_attribute_description(pipeline_desc.attribute_desc_count, pipeline_desc.attribute_descs);
    pipeline_desc.rasterizer_desc.cull_mode    = CullMode::eNone;
    pipeline_desc.rasterizer_desc.front_face   = FrontFace::eClockwise;
    pipeline_desc.depth_state_desc.depth_test  = true;
    pipeline_desc.depth_state_desc.depth_write = true;
    pipeline_desc.depth_state_desc.compare_op  = CompareOp::eLess;
    pipeline_desc.descriptor_set_layout        = &descriptor_set_layout;
    pipeline_desc.render_pass                  = &swapchain.render_passes[ 0 ];

    pipeline = create_graphics_pipeline(device, pipeline_desc);

    for (u32 i = 0; i < 2; ++i)
    {
        destroy_shader(device, shaders[ i ]);
    }

    SamplerDesc sampler_desc{};
    sampler_desc.mipmap_mode = SamplerMipmapMode::eLinear;
    sampler_desc.min_lod     = 0;
    sampler_desc.max_lod     = 1000;

    sampler = create_sampler(device, sampler_desc);

    camera_ubo.projection = create_perspective_matrix(radians(45.0f), window_get_aspect(get_app_window()), 0.1f, 100.f);
    camera_ubo.view = create_look_at_matrix(Vector3(0.0f, 0.0, 2.0f), Vector3(0.0, 0.0, -1.0), Vector3(0.0, 1.0, 0.0));

    BufferDesc buffer_desc{};
    buffer_desc.descriptor_type = DescriptorType::eUniformBuffer;
    buffer_desc.size            = sizeof(CameraUBO);
    // buffer_desc.data            = &camera_ubo;

    uniform_buffer = create_buffer(device, buffer_desc);

    DescriptorSetDesc descriptor_set_desc{};
    descriptor_set_desc.descriptor_set_layout = &descriptor_set_layout;

    descriptor_set = create_descriptor_set(device, descriptor_set_desc);

    std::vector<DescriptorImageDesc> descriptor_image_descs(model.textures.size());
    for (uint32_t i = 0; i < descriptor_image_descs.size(); ++i)
    {
        descriptor_image_descs[ i ]                = {};
        descriptor_image_descs[ i ].image          = &model.textures[ i ];
        descriptor_image_descs[ i ].resource_state = ResourceState::eShaderReadOnly;
    }

    DescriptorImageDesc descriptor_sampler_desc{};
    descriptor_sampler_desc.sampler = &sampler;

    DescriptorBufferDesc descriptor_buffer_desc{};
    descriptor_buffer_desc.buffer = &uniform_buffer;
    descriptor_buffer_desc.offset = 0;
    descriptor_buffer_desc.range  = sizeof(CameraUBO);

    DescriptorWriteDesc descriptor_writes[ 3 ]     = {};
    descriptor_writes[ 0 ].binding                 = 0;
    descriptor_writes[ 0 ].descriptor_count        = 1;
    descriptor_writes[ 0 ].descriptor_type         = DescriptorType::eUniformBuffer;
    descriptor_writes[ 0 ].descriptor_buffer_descs = &descriptor_buffer_desc;

    descriptor_writes[ 1 ].binding                = 1;
    descriptor_writes[ 1 ].descriptor_count       = 1;
    descriptor_writes[ 1 ].descriptor_type        = DescriptorType::eSampler;
    descriptor_writes[ 1 ].descriptor_image_descs = &descriptor_sampler_desc;

    descriptor_writes[ 2 ].binding                = 2;
    descriptor_writes[ 2 ].descriptor_count       = descriptor_image_descs.size();
    descriptor_writes[ 2 ].descriptor_image_descs = descriptor_image_descs.data();
    descriptor_writes[ 2 ].descriptor_type        = DescriptorType::eSampledImage;

    update_descriptor_set(device, descriptor_set, 3, descriptor_writes);
}

void on_resize(u32 width, u32 height)
{
    queue_wait_idle(queue);
    resize_swapchain(device, swapchain, width, height);

    camera_ubo.projection = create_perspective_matrix(radians(45.0f), window_get_aspect(get_app_window()), 0.1f, 100.f);
    camera_ubo.view = create_look_at_matrix(Vector3(0.0f, 0.0, 2.0f), Vector3(0.0, 0.0, -1.0), Vector3(0.0, 1.0, 0.0));

    BufferUpdateDesc buffer_update{};
    buffer_update.buffer = &uniform_buffer;
    buffer_update.offset = 0;
    buffer_update.size   = sizeof(CameraUBO);
    buffer_update.data   = &camera_ubo;

    update_buffer(device, buffer_update);
}

void on_update(f32 delta_time)
{
    if (!command_buffers_recorded[ frame_index ])
    {
        wait_for_fences(device, 1, &in_flight_fences[ frame_index ]);
        reset_fences(device, 1, &in_flight_fences[ frame_index ]);
        command_buffers_recorded[ frame_index ] = true;
    }

    u32 image_index = 0;
    acquire_next_image(device, swapchain, image_available_semaphores[ frame_index ], {}, image_index);

    auto& cmd = command_buffers[ frame_index ];

    begin_command_buffer(cmd);

    ImageBarrier to_clear_barrier{};
    to_clear_barrier.src_queue = &queue;
    to_clear_barrier.dst_queue = &queue;
    to_clear_barrier.image     = &swapchain.images[ image_index ];
    to_clear_barrier.old_state = ResourceState::eUndefined;
    to_clear_barrier.new_state = ResourceState::eColorAttachment;

    cmd_barrier(cmd, 0, nullptr, 1, &to_clear_barrier);

    RenderPassBeginDesc render_pass_begin_desc{};
    render_pass_begin_desc.render_pass                  = get_swapchain_render_pass(swapchain, image_index);
    render_pass_begin_desc.clear_values[ 0 ].color[ 0 ] = 1.0f;
    render_pass_begin_desc.clear_values[ 0 ].color[ 1 ] = 0.8f;
    render_pass_begin_desc.clear_values[ 0 ].color[ 2 ] = 0.4f;
    render_pass_begin_desc.clear_values[ 0 ].color[ 3 ] = 1.0f;
    render_pass_begin_desc.clear_values[ 1 ].depth      = 1.0f;
    render_pass_begin_desc.clear_values[ 1 ].stencil    = 0;

    cmd_begin_render_pass(cmd, render_pass_begin_desc);
    cmd_set_viewport(cmd, 0, 0, swapchain.width, swapchain.height, 0.0f, 1.0f);
    cmd_set_scissor(cmd, 0, 0, swapchain.width, swapchain.height);
    cmd_bind_pipeline(cmd, pipeline);
    cmd_bind_descriptor_set(cmd, descriptor_set, pipeline);

    PushConstantBlock pcb;
    pcb.model = Matrix4(1.0);
    pcb.model = translate(pcb.model, Vector3(0.0, 0.0, 0.0));
    pcb.model = glm::scale(pcb.model, Vector3(0.3));
    pcb.model = rotate(pcb.model, radians(get_time() / 10), Vector3(0.0, 1.0, 0.0));

    for (auto& mesh : model.meshes)
    {
        texture_indices.diffuse = mesh.material.diffuse;
        texture_indices.normal  = mesh.material.normal;
        cmd_push_constants(cmd, pipeline, 0, sizeof(PushConstantBlock), &pcb);
        cmd_push_constants(cmd, pipeline, sizeof(PushConstantBlock), sizeof(TextureIndices), &texture_indices);
        cmd_bind_vertex_buffer(cmd, mesh.vertex_buffer);
        cmd_bind_index_buffer_u32(cmd, mesh.index_buffer);
        cmd_draw_indexed(cmd, mesh.indices.size(), 1, 0, 0, 0);
    }
    cmd_end_render_pass(cmd);

    ImageBarrier to_present_barrier{};
    to_present_barrier.src_queue = &queue;
    to_present_barrier.dst_queue = &queue;
    to_present_barrier.image     = &swapchain.images[ image_index ];
    to_present_barrier.old_state = ResourceState::eColorAttachment;
    to_present_barrier.new_state = ResourceState::ePresent;

    cmd_barrier(cmd, 0, nullptr, 1, &to_present_barrier);

    end_command_buffer(cmd);

    QueueSubmitDesc queue_submit_desc{};
    queue_submit_desc.wait_semaphore_count   = 1;
    queue_submit_desc.wait_semaphores        = &image_available_semaphores[ frame_index ];
    queue_submit_desc.command_buffer_count   = 1;
    queue_submit_desc.command_buffers        = &cmd;
    queue_submit_desc.signal_semaphore_count = 1;
    queue_submit_desc.signal_semaphores      = &rendering_finished_semaphores[ frame_index ];
    queue_submit_desc.signal_fence           = &in_flight_fences[ frame_index ];

    queue_submit(queue, queue_submit_desc);

    QueuePresentDesc queue_present_desc{};
    queue_present_desc.wait_semaphore_count = 1;
    queue_present_desc.wait_semaphores      = &rendering_finished_semaphores[ frame_index ];
    queue_present_desc.swapchain            = &swapchain;
    queue_present_desc.image_index          = image_index;

    queue_present(queue, queue_present_desc);

    command_buffers_recorded[ frame_index ] = false;
    frame_index                             = (frame_index + 1) % FRAME_COUNT;
}

void on_shutdown()
{
    device_wait_idle(device);
    destroy_sampler(device, sampler);
    destroy_model(device, model);
    destroy_buffer(device, uniform_buffer);
    destroy_pipeline(device, pipeline);
    destroy_descriptor_set_layout(device, descriptor_set_layout);
    destroy_swapchain(device, swapchain);
    for (u32 i = 0; i < FRAME_COUNT; ++i)
    {
        destroy_semaphore(device, image_available_semaphores[ i ]);
        destroy_semaphore(device, rendering_finished_semaphores[ i ]);
        destroy_fence(device, in_flight_fences[ i ]);
    }
    free_command_buffers(device, command_pool, FRAME_COUNT, command_buffers);
    destroy_command_pool(device, command_pool);
    destroy_device(device);
    destroy_renderer(renderer);
}

int main(int argc, char** argv)
{
    ApplicationConfig config;
    config.argc        = argc;
    config.argv        = argv;
    config.title       = "TestApp";
    config.x           = 0;
    config.y           = 0;
    config.width       = 800;
    config.height      = 600;
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
