#pragma once

#include "volk.h"
#include "core/base.hpp"

namespace fluent
{

struct RendererDescription
{
    VkAllocationCallbacks* vulkan_allocator;
};

struct Renderer
{
    VkAllocationCallbacks* m_vulkan_allocator;
    VkInstance m_instance;
    VkDebugUtilsMessengerEXT m_debug_messenger;
    VkPhysicalDevice m_physical_device;
};

Renderer create_renderer(const RendererDescription& description);
void destroy_renderer(Renderer& renderer);

} // namespace fluent