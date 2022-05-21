#pragma once

#ifdef VULKAN_BACKEND
#include <volk/volk.h>
#include <vk_mem_alloc/vk_mem_alloc.h>
#include "renderer/renderer_backend.h"

struct VulkanRendererBackend
{
	VkAllocationCallbacks*   vulkan_allocator;
	VkInstance               instance;
	VkDebugUtilsMessengerEXT debug_messenger;
	VkPhysicalDevice         physical_device;
	u32                      api_version;
	struct RendererBackend   interface;
};

struct VulkanDevice
{
	u32                    index;
	VkAllocationCallbacks* vulkan_allocator;
	VkInstance             instance;
	VkPhysicalDevice       physical_device;
	VkDevice               logical_device;
	VmaAllocator           memory_allocator;
	VkDescriptorPool       descriptor_pool;
	struct Device          interface;
};

struct VulkanCommandPool
{
	VkCommandPool      command_pool;
	struct CommandPool interface;
};

struct VulkanCommandBuffer
{
	VkCommandBuffer      command_buffer;
	struct CommandBuffer interface;
};

struct VulkanQueue
{
	VkQueue      queue;
	struct Queue interface;
};

struct VulkanSemaphore
{
	VkSemaphore      semaphore;
	struct Semaphore interface;
};

struct VulkanFence
{
	VkFence      fence;
	struct Fence interface;
};

struct VulkanSampler
{
	VkSampler      sampler;
	struct Sampler interface;
};

struct VulkanImage
{
	VkImage       image;
	VkImageView   image_view;
	VmaAllocation allocation;
	struct Image  interface;
};

struct VulkanBuffer
{
	VkBuffer      buffer;
	VmaAllocation allocation;
	struct Buffer interface;
};

struct VulkanSwapchain
{
	VkPresentModeKHR              present_mode;
	VkColorSpaceKHR               color_space;
	VkSurfaceTransformFlagBitsKHR pre_transform;
	VkSurfaceKHR                  surface;
	VkSwapchainKHR                swapchain;
	struct Swapchain              interface;
};

struct VulkanShader
{
	VkShaderModule shaders[ FT_SHADER_STAGE_COUNT ];
	struct Shader  interface;
};

struct VulkanDescriptorSetLayout
{
	u32                        descriptor_set_layout_count;
	VkDescriptorSetLayout      descriptor_set_layouts[ MAX_SET_COUNT ];
	struct DescriptorSetLayout interface;
};

struct VulkanPipeline
{
	VkPipelineLayout pipeline_layout;
	VkPipeline       pipeline;
	struct Pipeline  interface;
};

struct VulkanDescriptorSet
{
	VkDescriptorSet      descriptor_set;
	struct DescriptorSet interface;
};

void
vk_create_renderer_backend( const struct RendererBackendInfo* info,
                            struct RendererBackend**          backend );

#endif
