#pragma once

#ifdef VULKAN_BACKEND
#include <volk.h>
#include <vk_mem_alloc.h>
#include "renderer/renderer_backend.hpp"

namespace fluent
{
struct VulkanRendererBackend
{
    VkAllocationCallbacks*   vulkan_allocator;
    VkInstance               instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkPhysicalDevice         physical_device;
    u32                      api_version;
    RendererBackend          interface;
};

struct VulkanDevice
{
    VkAllocationCallbacks* vulkan_allocator;
    VkInstance             instance;
    VkPhysicalDevice       physical_device;
    VkDevice               logical_device;
    VmaAllocator           memory_allocator;
    VkDescriptorPool       descriptor_pool;
    Device                 interface;
};

struct VulkanCommandPool
{
    VkCommandPool command_pool;
    CommandPool   interface;
};

struct VulkanCommandBuffer
{
    VkCommandBuffer command_buffer;
    CommandBuffer   interface;
};

struct VulkanQueue
{
    VkQueue queue;
    Queue   interface;
};

struct VulkanSemaphore
{
    VkSemaphore semaphore;
    Semaphore   interface;
};

struct VulkanFence
{
    VkFence fence = VK_NULL_HANDLE;
    Fence   interface;
};

struct VulkanSampler
{
    VkSampler sampler;
    Sampler   interface;
};

struct VulkanImage
{
    VkImage       image;
    VkImageView   image_view;
    VmaAllocation allocation;
    Image         interface;
};

struct VulkanBuffer
{
    VkBuffer      buffer;
    VmaAllocation allocation;
    Buffer        interface;
};

struct VulkanSwapchain
{
    VkPresentModeKHR              present_mode;
    VkColorSpaceKHR               color_space;
    VkSurfaceTransformFlagBitsKHR pre_transform;
    VkSurfaceKHR                  surface;
    VkSwapchainKHR                swapchain;
    Swapchain                     interface;
};

struct VulkanRenderPass
{
    VkRenderPass  render_pass;
    VkFramebuffer framebuffer;
    RenderPass    interface;
};

struct VulkanShader
{
    VkShaderModule shader;
    Shader         interface;
};

struct VulkanDescriptorSetLayout
{
    VkDescriptorSetLayout descriptor_set_layouts[ MAX_SET_COUNT ];
    DescriptorSetLayout   interface;
};

struct VulkanPipeline
{
    VkPipelineLayout pipeline_layout;
    VkPipeline       pipeline;
    Pipeline         interface;
};

struct VulkanDescriptorSet
{
    VkDescriptorSet descriptor_set;
    DescriptorSet   interface;
};

struct VulkanUiContext
{
    VkDescriptorPool desriptor_pool;
    UiContext        interface;
};

void
vk_create_renderer_backend( const RendererBackendDesc* desc,
                            RendererBackend**          p_backend );
} // namespace fluent
#endif
