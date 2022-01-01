#include <iostream>

#include "fluent/fluent.hpp"

using namespace fluent;

static constexpr u32 IMAGE_COUNT = 2;

Renderer g_renderer;
Device g_device;
Queue g_queue;
CommandPool g_command_pool;
Swapchain g_swapchain;
CommandBuffer g_command_buffers[ IMAGE_COUNT ];

u32 current_frame_index = 0;

void on_init()
{
    RendererDescription renderer_description{};
    renderer_description.vulkan_allocator = nullptr;
    g_renderer = create_renderer(renderer_description);

    DeviceDescription device_description{};
    g_device = create_device(g_renderer, device_description);

    QueueDescription queue_description{};
    queue_description.queue_type = QueueType::eGraphics;
    g_queue = get_queue(g_device, queue_description);

    CommandPoolDescription command_pool_description{};
    command_pool_description.queue = &g_queue;
    g_command_pool = create_command_pool(g_device, command_pool_description);

    allocate_command_buffers(g_device, g_command_pool, IMAGE_COUNT, g_command_buffers);
}

void on_load(u32 width, u32 height)
{
    SwapchainDescription swapchain_description{};
    swapchain_description.width = width;
    swapchain_description.height = height;
    swapchain_description.queue = &g_queue;
    swapchain_description.image_count = IMAGE_COUNT;

    g_swapchain = create_swapchain(g_renderer, g_device, swapchain_description);
}

void on_update(f64 deltaTime)
{
}

void on_render()
{
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
    present_to_clear.srcQueueFamilyIndex = g_queue.m_family_index;
    present_to_clear.dstQueueFamilyIndex - g_queue.m_family_index;
    present_to_clear.image = g_swapchain.m_images[ current_frame_index ].m_image;
    present_to_clear.subresourceRange = image_subresource_range;

    VkImageMemoryBarrier clear_to_present = {};
    clear_to_present.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    clear_to_present.pNext = nullptr;
    clear_to_present.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    clear_to_present.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    clear_to_present.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    clear_to_present.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    clear_to_present.srcQueueFamilyIndex = g_queue.m_family_index;
    clear_to_present.dstQueueFamilyIndex - g_queue.m_family_index;
    clear_to_present.image = g_swapchain.m_images[ current_frame_index ].m_image;
    clear_to_present.subresourceRange = image_subresource_range;
#endif

    begin_command_buffer(g_command_buffers[ current_frame_index ]);
#ifdef DEBUG_TESTING

    vkCmdPipelineBarrier(
        g_command_buffers[ current_frame_index ].m_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &present_to_clear);

    vkCmdClearColorImage(
        g_command_buffers[ current_frame_index ].m_command_buffer, g_swapchain.m_images[ current_frame_index ].m_image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color, 1, &image_subresource_range);

    vkCmdPipelineBarrier(
        g_command_buffers[ current_frame_index ].m_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &clear_to_present);
#endif
    end_command_buffer(g_command_buffers[ current_frame_index ]);

    current_frame_index = (current_frame_index + 1) % IMAGE_COUNT;
}

void on_unload()
{
    destroy_swapchain(g_device, g_swapchain);
}

void on_shutdown()
{
    free_command_buffers(g_device, g_command_pool, IMAGE_COUNT, g_command_buffers);
    destroy_command_pool(g_device, g_command_pool);
    destroy_device(g_device);
    destroy_renderer(g_renderer);
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
