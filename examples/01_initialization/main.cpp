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

void on_init()
{
    RendererDescription renderer_description{};
    renderer_description.vulkan_allocator = nullptr;
    renderer = create_renderer(renderer_description);

    DeviceDescription device_description{};
    device = create_device(renderer, device_description);

    QueueDescription queue_description{};
    queue_description.queue_type = QueueType::eGraphics;
    queue = get_queue(device, queue_description);

    CommandPoolDescription command_pool_description{};
    command_pool_description.queue = &queue;
    command_pool = create_command_pool(device, command_pool_description);

    allocate_command_buffers(device, command_pool, FRAME_COUNT, command_buffers);

    for (u32 i = 0; i < FRAME_COUNT; ++i)
    {
        image_available_semaphores[ i ] = create_semaphore(device);
        rendering_finished_semaphores[ i ] = create_semaphore(device);
        in_flight_fences[ i ] = create_fence(device);
        command_buffers_recorded[ i ] = false;
    }
}

void on_load(u32 width, u32 height)
{
    SwapchainDescription swapchain_description{};
    swapchain_description.width = width;
    swapchain_description.height = height;
    swapchain_description.queue = &queue;
    swapchain_description.image_count = FRAME_COUNT;

    swapchain = create_swapchain(renderer, device, swapchain_description);
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
    to_clear_barrier.src_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
    to_clear_barrier.dst_access_mask = VK_ACCESS_MEMORY_READ_BIT;
    to_clear_barrier.src_queue = &queue;
    to_clear_barrier.dst_queue = &queue;
    to_clear_barrier.image = &swapchain.m_images[ image_index ];
    to_clear_barrier.old_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    to_clear_barrier.new_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    cmd_barrier(command_buffers[ frame_index ], 0, nullptr, 1, &to_clear_barrier);

    f32 clear_value[ 4 ] = { 0.2f, 0.3f, 0.4f, 1.0f };

    RenderPassInfo render_pass_info{};
    render_pass_info.color_attachment_count = 1;
    render_pass_info.color_attachments[ 0 ] = &swapchain.m_images[ image_index ];
    render_pass_info.color_attachment_load_ops[ 0 ] = AttachmentLoadOp::eClear;
    render_pass_info.color_clear_values[ 0 ].color[ 0 ] = 1.0f;
    render_pass_info.color_clear_values[ 0 ].color[ 1 ] = 0.8f;
    render_pass_info.color_clear_values[ 0 ].color[ 2 ] = 0.4f;
    render_pass_info.color_clear_values[ 0 ].color[ 3 ] = 1.0f;
    render_pass_info.color_image_layouts[ 0 ] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    render_pass_info.depth_stencil = nullptr;
    render_pass_info.width = swapchain.m_width;
    render_pass_info.height = swapchain.m_height;

    cmd_begin_render_pass(device, command_buffers[ frame_index ], render_pass_info);
    cmd_end_render_pass(command_buffers[ frame_index ]);

    ImageBarrier to_present_barrier{};
    to_present_barrier.src_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
    to_present_barrier.dst_access_mask = VK_ACCESS_MEMORY_READ_BIT;
    to_present_barrier.src_queue = &queue;
    to_present_barrier.dst_queue = &queue;
    to_present_barrier.image = &swapchain.m_images[ image_index ];
    to_present_barrier.old_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    to_present_barrier.new_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    cmd_barrier(command_buffers[ frame_index ], 0, nullptr, 1, &to_present_barrier);

    end_command_buffer(command_buffers[ frame_index ]);

    QueueSubmitDescription queue_submit_description{};
    queue_submit_description.wait_semaphore_count = 1;
    queue_submit_description.wait_semaphores = &image_available_semaphores[ frame_index ];
    queue_submit_description.command_buffer_count = 1;
    queue_submit_description.command_buffers = &command_buffers[ frame_index ];
    queue_submit_description.signal_semaphore_count = 1;
    queue_submit_description.signal_semaphores = &rendering_finished_semaphores[ frame_index ];
    queue_submit_description.signal_fence = &in_flight_fences[ frame_index ];

    queue_submit(queue, queue_submit_description);

    QueuePresentDescription queue_present_description{};
    queue_present_description.wait_semaphore_count = 1;
    queue_present_description.wait_semaphores = &rendering_finished_semaphores[ frame_index ];
    queue_present_description.swapchain = &swapchain;
    queue_present_description.image_index = image_index;

    queue_present(queue, queue_present_description);

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
