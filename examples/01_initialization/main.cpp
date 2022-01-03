#include <iostream>

#include "fluent/fluent.hpp"

using namespace fluent;

static constexpr u32 FRAME_COUNT = 2;
u32 frame_index = 0;

Renderer renderer;
Device device;
Queue queue;
CommandPool command_pool;
Semaphore image_available_semaphores[ FRAME_COUNT ];
Semaphore rendering_finished_semaphores[ FRAME_COUNT ];
Fence in_flight_fences[ FRAME_COUNT ];
bool command_buffers_recorded[ FRAME_COUNT ];

Swapchain swapchain;
CommandBuffer command_buffers[ FRAME_COUNT ];

DescriptorSetLayout descriptor_set_layout;
Pipeline pipeline;

void on_init()
{
    RendererDesc renderer_desc{};
    renderer_desc.vulkan_allocator = nullptr;
    renderer = create_renderer(renderer_desc);

    DeviceDesc device_desc{};
    device = create_device(renderer, device_desc);

    QueueDesc queue_desc{};
    queue_desc.queue_type = QueueType::eGraphics;
    queue = get_queue(device, queue_desc);

    CommandPoolDesc command_pool_desc{};
    command_pool_desc.queue = &queue;
    command_pool = create_command_pool(device, command_pool_desc);

    allocate_command_buffers(device, command_pool, FRAME_COUNT, command_buffers);

    for (u32 i = 0; i < FRAME_COUNT; ++i)
    {
        image_available_semaphores[ i ] = create_semaphore(device);
        rendering_finished_semaphores[ i ] = create_semaphore(device);
        in_flight_fences[ i ] = create_fence(device);
        command_buffers_recorded[ i ] = false;
    }

    ShaderDesc vert_shader_desc{};
    vert_shader_desc.filename = "../examples/shaders/main.vert.glsl.spv";
    vert_shader_desc.stage = ShaderStage::eVertex;

    ShaderDesc frag_shader_desc{};
    frag_shader_desc.filename = "../examples/shaders/main.frag.glsl.spv";
    frag_shader_desc.stage = ShaderStage::eFragment;

    Shader shaders[ 2 ];
    shaders[ 0 ] = create_shader(device, vert_shader_desc);
    shaders[ 1 ] = create_shader(device, frag_shader_desc);

    DescriptorSetLayoutDesc descriptor_set_layout_desc{};
    descriptor_set_layout_desc.m_shader_count = 2;
    descriptor_set_layout_desc.m_shaders = shaders;

    descriptor_set_layout = create_descriptor_set_layout(device, descriptor_set_layout_desc);

    PipelineDesc pipeline_desc{};
    pipeline_desc.pipeline_type = PipelineType::eGraphics;
    pipeline_desc.binding_desc_count = 0;
    pipeline_desc.binding_descs = nullptr;
    pipeline_desc.attribute_desc_count = 0;
    pipeline_desc.attribute_descs = nullptr;
    pipeline_desc.rasterizer_desc.cull_mode = CullMode::eBack;
    pipeline_desc.rasterizer_desc.front_face = FrontFace::eCounterClockwise;
    pipeline_desc.depth_state_desc.depth_test = false;
    pipeline_desc.depth_state_desc.depth_write = false;
    pipeline_desc.descriptor_set_layout = &descriptor_set_layout;

    pipeline = create_pipeline(device, pipeline_desc);

    for (u32 i = 0; i < 2; ++i)
    {
        destroy_shader(device, shaders[ i ]);
    }
}

void on_load(u32 width, u32 height)
{
    SwapchainDesc swapchain_desc{};
    swapchain_desc.width = width;
    swapchain_desc.height = height;
    swapchain_desc.queue = &queue;
    swapchain_desc.image_count = FRAME_COUNT;

    swapchain = create_swapchain(renderer, device, swapchain_desc);
}

void on_update(f64 deltaTime)
{
}

void on_render()
{
    if (!command_buffers_recorded[ frame_index ])
    {
        wait_for_fences(device, 1, &in_flight_fences[ frame_index ]);
        reset_fences(device, 1, &in_flight_fences[ frame_index ]);
        command_buffers_recorded[ frame_index ] = true;
    }

    u32 image_index = 0;
    acquire_next_image(device, swapchain, image_available_semaphores[ frame_index ], {}, image_index);
    begin_command_buffer(command_buffers[ frame_index ]);

    ImageBarrier to_clear_barrier{};
    to_clear_barrier.src_queue = &queue;
    to_clear_barrier.dst_queue = &queue;
    to_clear_barrier.image = &swapchain.m_images[ image_index ];
    to_clear_barrier.old_state = ResourceState::eUndefined;
    to_clear_barrier.new_state = ResourceState::eColorAttachment;

    cmd_barrier(command_buffers[ frame_index ], 0, nullptr, 1, &to_clear_barrier);

    RenderPassBeginDesc render_pass_begin_desc{};
    render_pass_begin_desc.render_pass = get_swapchain_render_pass(swapchain, image_index);
    render_pass_begin_desc.clear_values[ 0 ].color[ 0 ] = 1.0f;
    render_pass_begin_desc.clear_values[ 0 ].color[ 1 ] = 0.8f;
    render_pass_begin_desc.clear_values[ 0 ].color[ 2 ] = 0.4f;
    render_pass_begin_desc.clear_values[ 0 ].color[ 3 ] = 1.0f;

    cmd_begin_render_pass(command_buffers[ frame_index ], render_pass_begin_desc);
    cmd_end_render_pass(command_buffers[ frame_index ]);

    ImageBarrier to_present_barrier{};
    to_present_barrier.src_queue = &queue;
    to_present_barrier.dst_queue = &queue;
    to_present_barrier.image = &swapchain.m_images[ image_index ];
    to_present_barrier.old_state = ResourceState::eColorAttachment;
    to_present_barrier.new_state = ResourceState::ePresent;

    cmd_barrier(command_buffers[ frame_index ], 0, nullptr, 1, &to_present_barrier);

    end_command_buffer(command_buffers[ frame_index ]);

    QueueSubmitDesc queue_submit_desc{};
    queue_submit_desc.wait_semaphore_count = 1;
    queue_submit_desc.wait_semaphores = &image_available_semaphores[ frame_index ];
    queue_submit_desc.command_buffer_count = 1;
    queue_submit_desc.command_buffers = &command_buffers[ frame_index ];
    queue_submit_desc.signal_semaphore_count = 1;
    queue_submit_desc.signal_semaphores = &rendering_finished_semaphores[ frame_index ];
    queue_submit_desc.signal_fence = &in_flight_fences[ frame_index ];

    queue_submit(queue, queue_submit_desc);

    QueuePresentDesc queue_present_desc{};
    queue_present_desc.wait_semaphore_count = 1;
    queue_present_desc.wait_semaphores = &rendering_finished_semaphores[ frame_index ];
    queue_present_desc.swapchain = &swapchain;
    queue_present_desc.image_index = image_index;

    queue_present(queue, queue_present_desc);

    command_buffers_recorded[ frame_index ] = false;
    frame_index = (frame_index + 1) % FRAME_COUNT;
}

void on_unload()
{
    queue_wait_idle(queue);
    destroy_swapchain(device, swapchain);
}

void on_shutdown()
{
    device_wait_idle(device);
    destroy_pipeline(device, pipeline);
    destroy_descriptor_set_layout(device, descriptor_set_layout);
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

int main()
{
    ApplicationConfig config;
    config.title = "TestApp";
    config.x = 0;
    config.y = 0;
    config.width = 800;
    config.height = 600;
    config.log_level = LogLevel::eTrace;
    config.on_init = on_init;
    config.on_load = on_load;
    config.on_update = on_update;
    config.on_render = on_render;
    config.on_unload = on_unload;
    config.on_shutdown = on_shutdown;

    application_init(&config);
    application_run();
    application_shutdown();

    return 0;
}
