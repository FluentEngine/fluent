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

Renderer create_renderer(const RendererDescription& description);
void destroy_renderer(Renderer& renderer);

Device create_device(const Renderer& renderer, const DeviceDescription& description);
void destroy_device(const Renderer& renderer, Device& device);

Queue get_queue(const Renderer& renderer, const Device& device, const QueueDescription& description);

Swapchain create_swapchain(const Renderer& renderer, const Device& device, const SwapchainDescription& description);
void destroy_swapchain(const Renderer& renderer, const Device& device, Swapchain& swapchain);

} // namespace fluent