#include <iostream>

#include "fluent/fluent.hpp"

#define DEBUG_TESTING

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
#ifdef DEBUG_TESTING
    VkClearColorValue clear_color = { { 1.0f, 0.8f, 0.4f, 0.0f } };

    VkImageSubresourceRange image_subresource_range = {};
    image_subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_subresource_range.baseMipLevel = 0;
    image_subresource_range.levelCount = 1;
    image_subresource_range.baseArrayLayer = 0;
    image_subresource_range.layerCount = 1;

    VkImageMemoryBarrier present_to_clear = {};
    present_to_clear.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    present_to_clear.pNext = nullptr;
    present_to_clear.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    present_to_clear.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    present_to_clear.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    present_to_clear.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    present_to_clear.srcQueueFamilyIndex = queue.m_family_index;
    present_to_clear.dstQueueFamilyIndex - queue.m_family_index;
    present_to_clear.image = swapchain.m_images[ image_index ].m_image;
    present_to_clear.subresourceRange = image_subresource_range;

    VkImageMemoryBarrier clear_to_present = {};
    clear_to_present.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    clear_to_present.pNext = nullptr;
    clear_to_present.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    clear_to_present.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    clear_to_present.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    clear_to_present.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    clear_to_present.srcQueueFamilyIndex = queue.m_family_index;
    clear_to_present.dstQueueFamilyIndex - queue.m_family_index;
    clear_to_present.image = swapchain.m_images[ image_index ].m_image;
    clear_to_present.subresourceRange = image_subresource_range;
#endif
    begin_command_buffer(command_buffers[ frame_index ]);
#ifdef DEBUG_TESTING
    vkCmdPipelineBarrier(
        command_buffers[ frame_index ].m_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &present_to_clear);

    vkCmdClearColorImage(
        command_buffers[ frame_index ].m_command_buffer, swapchain.m_images[ image_index ].m_image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color, 1, &image_subresource_range);

    vkCmdPipelineBarrier(
        command_buffers[ frame_index ].m_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &clear_to_present);
#endif
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
