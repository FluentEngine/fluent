#pragma once

#ifdef VULKAN_BACKEND
#include <volk/volk.h>
#include <vk_mem_alloc/vk_mem_alloc.h>
#include "../renderer_backend.h"

#ifdef FLUENT_DEBUG
#define VK_ASSERT( x )                                                         \
	do {                                                                       \
		VkResult err = x;                                                      \
		if ( err )                                                             \
		{                                                                      \
			FT_ERROR( "Detected Vulkan error: %s", err );                      \
			abort();                                                           \
		}                                                                      \
	} while ( 0 )
#else
#define VK_ASSERT( x ) x
#endif

struct vk_renderer_backend
{
	VkAllocationCallbacks*     vulkan_allocator;
	VkInstance                 instance;
	VkDebugUtilsMessengerEXT   debug_messenger;
	VkPhysicalDevice           physical_device;
	uint32_t                   api_version;
	struct ft_renderer_backend interface;
};

struct vk_device
{
	VkAllocationCallbacks* vulkan_allocator;
	VkInstance             instance;
	VkPhysicalDevice       physical_device;
	VkDevice               logical_device;
	VmaAllocator           memory_allocator;
	VkDescriptorPool       descriptor_pool;
	struct ft_device       interface;
};

struct vk_command_pool
{
	VkCommandPool          command_pool;
	struct ft_command_pool interface;
};

struct vk_command_buffer
{
	VkCommandBuffer          command_buffer;
	struct ft_command_buffer interface;
};

struct vk_queue
{
	VkQueue         queue;
	struct ft_queue interface;
};

struct vk_semaphore
{
	VkSemaphore         semaphore;
	struct ft_semaphore interface;
};

struct vk_fence
{
	VkFence         fence;
	struct ft_fence interface;
};

struct vk_sampler
{
	VkSampler         sampler;
	struct ft_sampler interface;
};

struct vk_image
{
	VkImage         image;
	VkImageView     sampled_view;
	VkImageView*    storage_views;
	VmaAllocation   allocation;
	struct ft_image interface;
};

struct vk_buffer
{
	VkBuffer         buffer;
	VmaAllocation    allocation;
	struct ft_buffer interface;
};

struct vk_swapchain
{
	VkPresentModeKHR              present_mode;
	VkColorSpaceKHR               color_space;
	VkSurfaceTransformFlagBitsKHR pre_transform;
	VkSurfaceKHR                  surface;
	VkSwapchainKHR                swapchain;
	struct ft_swapchain           interface;
};

struct vk_shader
{
	VkShaderModule   shaders[ FT_SHADER_STAGE_COUNT ];
	struct ft_shader interface;
};

struct vk_descriptor_set_layout
{
	uint32_t                        descriptor_set_layout_count;
	VkDescriptorSetLayout           descriptor_set_layouts[ FT_MAX_SET_COUNT ];
	struct ft_descriptor_set_layout interface;
};

struct vk_pipeline
{
	VkPipelineLayout   pipeline_layout;
	VkPipeline         pipeline;
	struct ft_pipeline interface;
};

struct vk_descriptor_set
{
	VkDescriptorSet          descriptor_set;
	struct ft_descriptor_set interface;
};

void
vk_create_renderer_backend( const struct ft_renderer_backend_info* info,
                            struct ft_renderer_backend**           backend );

#endif
