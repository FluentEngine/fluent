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
    VkQueue m_queue;
};

Renderer create_renderer(const RendererDescription& description);
void destroy_renderer(Renderer& renderer);

Device create_device(const Renderer& renderer, const DeviceDescription& description);
void destroy_device(const Renderer& renderer, Device& device);

Queue get_queue(const Renderer& renderer, const Device& device, const QueueDescription& description);

} // namespace fluent