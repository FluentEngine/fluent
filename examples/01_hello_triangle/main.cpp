#include <iostream>

#include "fluent/fluent.hpp"

using namespace fluent;

static constexpr u32 FRAME_COUNT = 2;
u32                  frame_index = 0;

RendererBackend* renderer;
Device*          device;
Queue*           queue;
CommandPool*     command_pool;
Semaphore*       image_available_semaphores[ FRAME_COUNT ];
Semaphore*       rendering_finished_semaphores[ FRAME_COUNT ];
Fence*           in_flight_fences[ FRAME_COUNT ];
bool             command_buffers_recorded[ FRAME_COUNT ];

Swapchain*     swapchain;
CommandBuffer* command_buffers[ FRAME_COUNT ];

DescriptorSetLayout* descriptor_set_layout;
Pipeline*            pipeline;

Buffer* vertex_buffer;

static const f32 vertices[] = { -0.5f, -0.5f, 0.5f, -0.5f, 0.0f, 0.5f };

void on_init()
{
    FileSystem::set_shaders_directory("../../../examples/shaders/01_hello_triangle");

    RendererBackendDesc renderer_desc{};
    renderer_desc.vulkan_allocator = nullptr;
    create_renderer_backend(&renderer_desc, &renderer);

    DeviceDesc device_desc{};
    device_desc.frame_in_use_count = 2;
    create_device(renderer, &device_desc, &device);

    QueueDesc queue_desc{};
    queue_desc.queue_type = QueueType::eGraphics;
    create_queue(device, &queue_desc, &queue);

    CommandPoolDesc command_pool_desc{};
    command_pool_desc.queue = queue;
    create_command_pool(device, &command_pool_desc, &command_pool);

    create_command_buffers(device, command_pool, FRAME_COUNT, command_buffers);

    for (u32 i = 0; i < FRAME_COUNT; ++i)
    {
        create_semaphore(device, &image_available_semaphores[ i ]);
        create_semaphore(device, &rendering_finished_semaphores[ i ]);
        create_fence(device, &in_flight_fences[ i ]);
        command_buffers_recorded[ i ] = false;
    }

    SwapchainDesc swapchain_desc{};
    swapchain_desc.width           = window_get_width(get_app_window());
    swapchain_desc.height          = window_get_height(get_app_window());
    swapchain_desc.queue           = queue;
    swapchain_desc.min_image_count = FRAME_COUNT;

    create_swapchain(device, &swapchain_desc, &swapchain);

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
    create_shader(device, &shader_descs[ 0 ], &shaders[ 0 ]);
    create_shader(device, &shader_descs[ 1 ], &shaders[ 1 ]);

    create_descriptor_set_layout(device, 2, shaders, &descriptor_set_layout);

    PipelineDesc  pipeline_desc{};
    VertexLayout& vertex_layout                 = pipeline_desc.vertex_layout;
    vertex_layout.binding_desc_count            = 1;
    vertex_layout.binding_descs[ 0 ].binding    = 0;
    vertex_layout.binding_descs[ 0 ].input_rate = VertexInputRate::eVertex;
    vertex_layout.binding_descs[ 0 ].stride     = 2 * sizeof(float);
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
    pipeline_desc.render_pass                   = swapchain->render_passes[ 0 ];

    create_graphics_pipeline(device, &pipeline_desc, &pipeline);

    for (u32 i = 0; i < 2; ++i)
    {
        destroy_shader(device, shaders[ i ]);
    }

    BufferDesc buffer_desc{};
    buffer_desc.size            = sizeof(vertices);
    buffer_desc.descriptor_type = DescriptorType::eVertexBuffer;
    buffer_desc.memory_usage    = MemoryUsage::eCpuToGpu;

    create_buffer(device, &buffer_desc, &vertex_buffer);
    map_memory(device, vertex_buffer);
    std::memcpy(vertex_buffer->mapped_memory, vertices, sizeof(vertices));
    unmap_memory(device, vertex_buffer);
}

void on_resize(u32 width, u32 height)
{
    queue_wait_idle(queue);
    resize_swapchain(device, swapchain, width, height);
}

void on_update(f32 delta_time)
{
    if (!command_buffers_recorded[ frame_index ])
    {
        wait_for_fences(device, 1, in_flight_fences[ frame_index ]);
        reset_fences(device, 1, in_flight_fences[ frame_index ]);
        command_buffers_recorded[ frame_index ] = true;
    }

    u32 image_index = 0;
    acquire_next_image(device, swapchain, image_available_semaphores[ frame_index ], nullptr, &image_index);

    auto& cmd = command_buffers[ frame_index ];

    begin_command_buffer(cmd);

    ImageBarrier to_clear_barrier{};
    to_clear_barrier.src_queue = queue;
    to_clear_barrier.dst_queue = queue;
    to_clear_barrier.image     = swapchain->images[ image_index ];
    to_clear_barrier.old_state = ResourceState::eUndefined;
    to_clear_barrier.new_state = ResourceState::eColorAttachment;

    cmd_barrier(cmd, 0, nullptr, 1, &to_clear_barrier);

    RenderPassBeginDesc render_pass_begin_desc{};
    render_pass_begin_desc.render_pass                  = get_swapchain_render_pass(swapchain, image_index);
    render_pass_begin_desc.clear_values[ 0 ].color[ 0 ] = 1.0f;
    render_pass_begin_desc.clear_values[ 0 ].color[ 1 ] = 0.8f;
    render_pass_begin_desc.clear_values[ 0 ].color[ 2 ] = 0.4f;
    render_pass_begin_desc.clear_values[ 0 ].color[ 3 ] = 1.0f;

    cmd_begin_render_pass(cmd, &render_pass_begin_desc);
    cmd_set_viewport(cmd, 0, 0, swapchain->width, swapchain->height, 0.0f, 1.0f);
    cmd_set_scissor(cmd, 0, 0, swapchain->width, swapchain->height);
    cmd_bind_pipeline(cmd, pipeline);
    cmd_bind_vertex_buffer(cmd, vertex_buffer);
    cmd_draw(cmd, 3, 1, 0, 0);
    cmd_end_render_pass(cmd);

    ImageBarrier to_present_barrier{};
    to_present_barrier.src_queue = queue;
    to_present_barrier.dst_queue = queue;
    to_present_barrier.image     = swapchain->images[ image_index ];
    to_present_barrier.old_state = ResourceState::eColorAttachment;
    to_present_barrier.new_state = ResourceState::ePresent;

    cmd_barrier(cmd, 0, nullptr, 1, &to_present_barrier);

    end_command_buffer(cmd);

    QueueSubmitDesc queue_submit_desc{};
    queue_submit_desc.wait_semaphore_count   = 1;
    queue_submit_desc.wait_semaphores        = image_available_semaphores[ frame_index ];
    queue_submit_desc.command_buffer_count   = 1;
    queue_submit_desc.command_buffers        = cmd;
    queue_submit_desc.signal_semaphore_count = 1;
    queue_submit_desc.signal_semaphores      = rendering_finished_semaphores[ frame_index ];
    queue_submit_desc.signal_fence           = in_flight_fences[ frame_index ];

    queue_submit(queue, &queue_submit_desc);

    QueuePresentDesc queue_present_desc{};
    queue_present_desc.wait_semaphore_count = 1;
    queue_present_desc.wait_semaphores      = rendering_finished_semaphores[ frame_index ];
    queue_present_desc.swapchain            = swapchain;
    queue_present_desc.image_index          = image_index;

    queue_present(queue, &queue_present_desc);

    command_buffers_recorded[ frame_index ] = false;
    frame_index                             = (frame_index + 1) % FRAME_COUNT;
}

void on_shutdown()
{
    device_wait_idle(device);
    destroy_buffer(device, vertex_buffer);
    destroy_pipeline(device, pipeline);
    destroy_descriptor_set_layout(device, descriptor_set_layout);
    destroy_swapchain(device, swapchain);
    for (u32 i = 0; i < FRAME_COUNT; ++i)
    {
        destroy_semaphore(device, image_available_semaphores[ i ]);
        destroy_semaphore(device, rendering_finished_semaphores[ i ]);
        destroy_fence(device, in_flight_fences[ i ]);
    }
    destroy_command_buffers(device, command_pool, FRAME_COUNT, command_buffers);
    destroy_command_pool(device, command_pool);
    destroy_queue(queue);
    destroy_device(device);
    destroy_renderer_backend(renderer);
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
