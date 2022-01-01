#pragma once

#include "volk.h"
#include "core/base.hpp"

namespace fluent
{

struct RendererDescription
{
    VkAllocationCallbacks* vulkan_allocator;
};

enum class QueueType : u8
{
    eGraphics = 0,
    eCompute = 1,
    eTransfer = 2,
    eLast
};

struct Renderer
{
    VkAllocationCallbacks* m_vulkan_allocator;
    VkInstance m_instance;
    VkDebugUtilsMessengerEXT m_debug_messenger;
    VkPhysicalDevice m_physical_device;
};

struct DeviceDescription
{
};

struct Device
{
    VkAllocationCallbacks* m_vulkan_allocator;
    VkInstance m_instance;
    VkPhysicalDevice m_physical_device;
    VkDevice m_logical_device;
};

struct QueueDescription
{
    QueueType queue_type;
};

struct Queue
{
    u32 m_family_index;
    VkQueue m_queue;
};

struct SwapchainDescription
{
    Queue* queue;
    u32 width;
    u32 height;
    u32 image_count;
};

struct Image
{
    VkImage m_image;
    VkImageView m_image_view;
};

struct Swapchain
{
    VkPresentModeKHR m_present_mode;
    u32 m_image_count;
    u32 m_width;
    u32 m_height;
    VkSurfaceKHR m_surface;
    VkSwapchainKHR m_swapchain;
    VkFormat m_format;
    Image* m_images;
};

struct CommandPoolDescription
{
    Queue* queue;
};

struct CommandPool
{
    VkCommandPool m_command_pool;
};

struct CommandBuffer
{
    VkCommandBuffer m_command_buffer;
};

Renderer create_renderer(const RendererDescription& description);
void destroy_renderer(Renderer& renderer);

Device create_device(const Renderer& renderer, const DeviceDescription& description);
void destroy_device(Device& device);

Queue get_queue(const Device& device, const QueueDescription& description);

Swapchain create_swapchain(const Renderer& renderer, const Device& device, const SwapchainDescription& description);
void destroy_swapchain(const Device& device, Swapchain& swapchain);

CommandPool create_command_pool(const Device& device, const CommandPoolDescription& description);
void destroy_command_pool(const Device& device, CommandPool& command_pool);

void allocate_command_buffers(
    const Device& device, const CommandPool& command_pool, u32 count, CommandBuffer* command_buffers);
void free_command_buffers(
    const Device& device, const CommandPool& command_pool, u32 count, CommandBuffer* command_buffers);
void begin_command_buffer(const CommandBuffer& command_buffer);
void end_command_buffer(const CommandBuffer& command_buffer);

} // namespace fluent