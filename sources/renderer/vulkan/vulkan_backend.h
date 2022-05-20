#pragma once

#ifdef VULKAN_BACKEND
#include <volk.h>
#include <vk_mem_alloc.h>
#include "renderer/renderer_backend.h"

typedef struct VulkanRendererBackend
{
	VkAllocationCallbacks*   vulkan_allocator;
	VkInstance               instance;
	VkDebugUtilsMessengerEXT debug_messenger;
	VkPhysicalDevice         physical_device;
	u32                      api_version;
	RendererBackend          interface;
} VulkanRendererBackend;

typedef struct VulkanDevice
{
	u32                    index;
	VkAllocationCallbacks* vulkan_allocator;
	VkInstance             instance;
	VkPhysicalDevice       physical_device;
	VkDevice               logical_device;
	VmaAllocator           memory_allocator;
	VkDescriptorPool       descriptor_pool;
	Device                 interface;
} VulkanDevice;

typedef struct VulkanCommandPool
{
	VkCommandPool command_pool;
	CommandPool   interface;
} VulkanCommandPool;

typedef struct VulkanCommandBuffer
{
	VkCommandBuffer command_buffer;
	CommandBuffer   interface;
} VulkanCommandBuffer;

typedef struct VulkanQueue
{
	VkQueue queue;
	Queue   interface;
} VulkanQueue;

typedef struct VulkanSemaphore
{
	VkSemaphore semaphore;
	Semaphore   interface;
} VulkanSemaphore;

typedef struct VulkanFence
{
	VkFence fence;
	Fence   interface;
} VulkanFence;

typedef struct VulkanSampler
{
	VkSampler sampler;
	Sampler   interface;
} VulkanSampler;

typedef struct VulkanImage
{
	VkImage       image;
	VkImageView   image_view;
	VmaAllocation allocation;
	Image         interface;
} VulkanImage;

typedef struct VulkanBuffer
{
	VkBuffer      buffer;
	VmaAllocation allocation;
	Buffer        interface;
} VulkanBuffer;

typedef struct VulkanSwapchain
{
	VkPresentModeKHR              present_mode;
	VkColorSpaceKHR               color_space;
	VkSurfaceTransformFlagBitsKHR pre_transform;
	VkSurfaceKHR                  surface;
	VkSwapchainKHR                swapchain;
	Swapchain                     interface;
} VulkanSwapchain;

typedef struct VulkanShader
{
	VkShaderModule shaders[ FT_SHADER_STAGE_COUNT ];
	Shader         interface;
} VulkanShader;

typedef struct VulkanDescriptorSetLayout
{
	u32                   descriptor_set_layout_count;
	VkDescriptorSetLayout descriptor_set_layouts[ MAX_SET_COUNT ];
	DescriptorSetLayout   interface;
} VulkanDescriptorSetLayout;

typedef struct VulkanPipeline
{
	VkPipelineLayout pipeline_layout;
	VkPipeline       pipeline;
	Pipeline         interface;
} VulkanPipeline;

typedef struct VulkanDescriptorSet
{
	VkDescriptorSet descriptor_set;
	DescriptorSet   interface;
} VulkanDescriptorSet;

void
vk_create_renderer_backend( const RendererBackendInfo* info,
                            RendererBackend**          backend );

#endif
