#ifdef VULKAN_BACKEND

#include <hashmap_c/hashmap_c.h>
#include "log/log.h"
#include "wsi/wsi.h"
#include "../renderer_private.h"
#include "../shader_reflection.h"
#include "vulkan_enum_translators.h"
#include "vulkan_pass_hasher.h"
#include "vulkan_backend.h"

#ifdef FLUENT_DEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL
vulkan_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT             flags,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void*                                       user_data )
{
	FT_UNUSED( flags );
	FT_UNUSED( pCallbackData );
	FT_UNUSED( user_data );

	static char const* prefix = "[Vulkan]:";
	FT_UNUSED( prefix );

	if ( messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT )
	{
		FT_TRACE( "%9s %s", prefix, pCallbackData->pMessage );
	}

	else if ( messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT )
	{
		FT_INFO( "%10s %s", prefix, pCallbackData->pMessage );
	}
	else if ( messageSeverity ==
	          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT )
	{
		FT_WARN( "%10s %s", prefix, pCallbackData->pMessage );
	}
	else if ( messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT )
	{
		FT_ERROR( "%9s %s", prefix, pCallbackData->pMessage );
	}

	return VK_FALSE;
}

FT_INLINE void
create_debug_messenger( struct vk_renderer_backend* backend )
{
	VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info = { 0 };
	debug_messenger_create_info.sType =
	    VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debug_messenger_create_info.messageSeverity =
	    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
	    VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
	    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
	    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debug_messenger_create_info.messageType =
	    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
	    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
	    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debug_messenger_create_info.pfnUserCallback = vulkan_debug_callback;
	debug_messenger_create_info.pUserData       = NULL;

	vkCreateDebugUtilsMessengerEXT( backend->instance,
	                                &debug_messenger_create_info,
	                                backend->vulkan_allocator,
	                                &backend->debug_messenger );
}
#endif

FT_INLINE void
get_instance_extensions( const struct ft_renderer_backend_info* info,
                         uint32_t*    instance_create_flags,
                         uint32_t*    count,
                         const char** names )
{
	*instance_create_flags = 0;

	struct ft_wsi_info* wsi = info->wsi_info;

	if ( names == NULL )
	{
		*count = wsi->vulkan_instance_extension_count;
	}

	uint32_t e = 0;
	if ( wsi->vulkan_instance_extension_count != 0 && names )
	{
		for ( e = 0; e < wsi->vulkan_instance_extension_count; ++e )
		{
			names[ e ] = wsi->vulkan_instance_extensions[ e ];
		}
	}
	else if ( names )
	{
		FT_WARN( "Wsi has no instance extensions. Present not supported" );
	}

	uint32_t extension_count = 0;
	vkEnumerateInstanceExtensionProperties( NULL, &extension_count, NULL );
	FT_ALLOC_STACK_ARRAY( VkExtensionProperties,
	                      extension_properties,
	                      extension_count );
	vkEnumerateInstanceExtensionProperties( NULL,
	                                        &extension_count,
	                                        extension_properties );

	for ( uint32_t i = 0; i < extension_count; ++i )
	{
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
		if ( strcmp( VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
		             extension_properties[ i ].extensionName ) == 0 )
		{
			if ( names )
			{
				names[ e++ ] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
				( *instance_create_flags ) |= VK_KHR_portability_enumeration;
			}
			else
			{
				( *count )++;
			}
		}
#endif
#ifdef FLUENT_DEBUG
		if ( strcmp( VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		             extension_properties[ i ].extensionName ) == 0 )
		{
			if ( names )
			{
				names[ e++ ] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
			}
			else
			{
				( *count )++;
			}
		}
#endif
	}
}

FT_INLINE void
get_instance_layers( uint32_t* count, const char** names )
{
	FT_UNUSED( names );

	*count = 0;
#ifdef FLUENT_DEBUG
	( *count )++;
	if ( names )
	{
		names[ 0 ] = "VK_LAYER_KHRONOS_validation";
	}
#endif
}

FT_INLINE uint32_t
find_queue_family_index( VkPhysicalDevice   physical_device,
                         enum ft_queue_type queue_type )
{
	uint32_t index = UINT32_MAX;

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties( physical_device,
	                                          &queue_family_count,
	                                          NULL );
	FT_ALLOC_STACK_ARRAY( VkQueueFamilyProperties,
	                      queue_families,
	                      queue_family_count );
	vkGetPhysicalDeviceQueueFamilyProperties( physical_device,
	                                          &queue_family_count,
	                                          queue_families );

	VkQueueFlagBits target_queue_type = to_vk_queue_type( queue_type );

	// TODO: Find dedicated queue first
	for ( uint32_t i = 0; i < queue_family_count; ++i )
	{
		if ( queue_families[ i ].queueFlags & target_queue_type )
		{
			index = i;
			break;
		}
	}

	// TODO: Rewrite more elegant way
	FT_ASSERT( index != UINT32_MAX );

	return index;
}

FT_INLINE VkImageAspectFlags
get_aspect_mask( enum ft_format format )
{
	VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;

	if ( ft_format_has_depth_aspect( format ) )
	{
		aspect_mask &= ~VK_IMAGE_ASPECT_COLOR_BIT;
		aspect_mask |= VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else if ( ft_format_has_stencil_aspect( format ) )
	{
		aspect_mask &= ~VK_IMAGE_ASPECT_COLOR_BIT;
		aspect_mask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	return aspect_mask;
}

FT_INLINE VkImageSubresourceRange
get_image_subresource_range( const struct vk_image* image )
{
	VkImageSubresourceRange image_subresource_range = {
	    .aspectMask     = get_aspect_mask( image->interface.format ),
	    .baseMipLevel   = 0,
	    .levelCount     = image->interface.mip_level_count,
	    .baseArrayLayer = 0,
	    .layerCount     = image->interface.layer_count,
	};

	return image_subresource_range;
}

FT_INLINE VkImageSubresourceLayers
get_image_subresource_layers( const struct vk_image* image )
{
	VkImageSubresourceRange subresourceRange =
	    get_image_subresource_range( image );

	VkImageSubresourceLayers layers = {
	    .aspectMask     = subresourceRange.aspectMask,
	    .mipLevel       = subresourceRange.baseMipLevel,
	    .baseArrayLayer = subresourceRange.baseArrayLayer,
	    .layerCount     = subresourceRange.layerCount,
	};

	return layers;
}

static void
vk_destroy_renderer_backend( struct ft_renderer_backend* ibackend )
{
	FT_FROM_HANDLE( backend, ibackend, vk_renderer_backend );

#ifdef FLUENT_DEBUG
	vkDestroyDebugUtilsMessengerEXT( backend->instance,
	                                 backend->debug_messenger,
	                                 backend->vulkan_allocator );
#endif
	vkDestroyInstance( backend->instance, backend->vulkan_allocator );
	free( backend );
}

static void*
vk_map_memory( const struct ft_device* idevice, struct ft_buffer* ibuffer )
{
	FT_FROM_HANDLE( device, idevice, vk_device );
	FT_FROM_HANDLE( buffer, ibuffer, vk_buffer );

	vmaMapMemory( device->memory_allocator,
	              buffer->allocation,
	              &buffer->interface.mapped_memory );

	return buffer->interface.mapped_memory;
}

static void
vk_unmap_memory( const struct ft_device* idevice, struct ft_buffer* ibuffer )
{
	FT_FROM_HANDLE( device, idevice, vk_device );
	FT_FROM_HANDLE( buffer, ibuffer, vk_buffer );

	vmaUnmapMemory( device->memory_allocator, buffer->allocation );
	buffer->interface.mapped_memory = NULL;
}

static void
vk_create_device( const struct ft_renderer_backend* ibackend,
                  const struct ft_device_info*      info,
                  struct ft_device**                p )
{
	FT_UNUSED( info );

	FT_FROM_HANDLE( backend, ibackend, vk_renderer_backend );

	FT_INIT_INTERNAL( device, *p, vk_device );

	device->vulkan_allocator = backend->vulkan_allocator;
	device->instance         = backend->instance;
	device->physical_device  = backend->physical_device;

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties( device->physical_device,
	                                          &queue_family_count,
	                                          NULL );
	FT_ALLOC_STACK_ARRAY( VkQueueFamilyProperties,
	                      queue_families,
	                      queue_family_count );
	vkGetPhysicalDeviceQueueFamilyProperties( device->physical_device,
	                                          &queue_family_count,
	                                          queue_families );

	uint32_t                queue_create_info_count = 0;
	VkDeviceQueueCreateInfo queue_create_infos[ FT_QUEUE_TYPE_COUNT ];
	float                   queue_priority = 1.0f;

	// TODO: Select queues
	for ( uint32_t i = 0; i < queue_family_count; ++i )
	{
		queue_create_infos[ queue_create_info_count ].sType =
		    VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_infos[ queue_create_info_count ].pNext      = NULL;
		queue_create_infos[ queue_create_info_count ].flags      = 0;
		queue_create_infos[ queue_create_info_count ].queueCount = 1;
		queue_create_infos[ queue_create_info_count ].pQueuePriorities =
		    &queue_priority;
		queue_create_infos[ queue_create_info_count ].queueFamilyIndex = i;
		queue_create_info_count++;

		if ( queue_create_info_count == FT_QUEUE_TYPE_COUNT )
			break;
	}

	uint32_t device_extension_count = 0;
	vkEnumerateDeviceExtensionProperties( device->physical_device,
	                                      NULL,
	                                      &device_extension_count,
	                                      NULL );
	FT_ALLOC_STACK_ARRAY( VkExtensionProperties,
	                      supported_device_extensions,
	                      device_extension_count + 2 ); // TODO:
	vkEnumerateDeviceExtensionProperties( device->physical_device,
	                                      NULL,
	                                      &device_extension_count,
	                                      supported_device_extensions );

	bool portability_subset = 0;
	for ( uint32_t i = 0; i < device_extension_count; ++i )
	{
		if ( !strcmp( supported_device_extensions[ i ].extensionName,
		              "VK_KHR_portability_subset" ) )
		{
			portability_subset = 1;
		}
	}

	device_extension_count = 2 + portability_subset;
	FT_ALLOC_STACK_ARRAY( const char*,
	                      device_extensions,
	                      device_extension_count );
	device_extensions[ 0 ] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
	device_extensions[ 1 ] = VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME;
	if ( portability_subset )
	{
		device_extensions[ 2 ] = "VK_KHR_portability_subset";
	}

	// TODO: check support
	VkPhysicalDeviceFeatures used_features = {
	    .fillModeNonSolid  = VK_TRUE,
	    .multiDrawIndirect = VK_TRUE,
	    .sampleRateShading = VK_TRUE,
	    .samplerAnisotropy = VK_TRUE,
	};

	// TODO: dont use partially bound
	VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features = {
	    .sType =
	        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
	    .descriptorBindingPartiallyBound = 1,
	};

	VkPhysicalDeviceMultiviewFeatures multiview_features = {
	    .sType     = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES,
	    .multiview = 1,
	    .pNext     = &descriptor_indexing_features,
	};

	VkPhysicalDeviceShaderDrawParametersFeatures shader_draw_parameters = {
	    .sType =
	        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETER_FEATURES,
	    .shaderDrawParameters = 1,
	    .pNext                = &multiview_features,
	};

	VkDeviceCreateInfo device_create_info = {
	    .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
	    .pNext                   = &shader_draw_parameters,
	    .flags                   = 0,
	    .queueCreateInfoCount    = queue_create_info_count,
	    .pQueueCreateInfos       = queue_create_infos,
	    .enabledLayerCount       = 0,
	    .ppEnabledLayerNames     = NULL,
	    .enabledExtensionCount   = device_extension_count,
	    .ppEnabledExtensionNames = device_extensions,
	    .pEnabledFeatures        = &used_features,
	};

	VK_ASSERT( vkCreateDevice( device->physical_device,
	                           &device_create_info,
	                           device->vulkan_allocator,
	                           &device->logical_device ) );

	volkLoadDevice( device->logical_device );

	VmaVulkanFunctions vulkan_functions = {
		.vkGetInstanceProcAddr             = vkGetInstanceProcAddr,
		.vkGetDeviceProcAddr               = vkGetDeviceProcAddr,
		.vkAllocateMemory                  = vkAllocateMemory,
		.vkBindBufferMemory                = vkBindBufferMemory,
		.vkBindImageMemory                 = vkBindImageMemory,
		.vkCreateBuffer                    = vkCreateBuffer,
		.vkCreateImage                     = vkCreateImage,
		.vkDestroyBuffer                   = vkDestroyBuffer,
		.vkDestroyImage                    = vkDestroyImage,
		.vkFreeMemory                      = vkFreeMemory,
		.vkGetBufferMemoryRequirements     = vkGetBufferMemoryRequirements,
		.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR,
		.vkGetImageMemoryRequirements      = vkGetImageMemoryRequirements,
		.vkGetImageMemoryRequirements2KHR  = vkGetImageMemoryRequirements2KHR,
		.vkGetPhysicalDeviceMemoryProperties =
		    vkGetPhysicalDeviceMemoryProperties,
		.vkGetPhysicalDeviceProperties  = vkGetPhysicalDeviceProperties,
		.vkMapMemory                    = vkMapMemory,
		.vkUnmapMemory                  = vkUnmapMemory,
		.vkFlushMappedMemoryRanges      = vkFlushMappedMemoryRanges,
		.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges,
		.vkCmdCopyBuffer                = vkCmdCopyBuffer,
#if VMA_BIND_MEMORY2 || VMA_VULKAN_VERSION >= 1001000
		.vkBindBufferMemory2KHR = vkBindBufferMemory2KHR,
		.vkBindImageMemory2KHR  = vkBindImageMemory2KHR,
#endif
#if VMA_MEMORY_BUDGET || VMA_VULKAN_VERSION >= 1001000
		.vkGetPhysicalDeviceMemoryProperties2KHR =
		    vkGetPhysicalDeviceMemoryProperties2KHR,
#endif
#if VMA_VULKAN_VERSION >= 1003000
		.vkGetDeviceBufferMemoryRequirements =
		    vkGetDeviceBufferMemoryRequirementsKHR,
		.vkGetDeviceImageMemoryRequirements =
		    vkGetDeviceImageMemoryRequirements,
#endif
	};

	VmaAllocatorCreateInfo vma_allocator_create_info = {
	    .instance             = device->instance,
	    .physicalDevice       = device->physical_device,
	    .device               = device->logical_device,
	    .flags                = 0,
	    .pAllocationCallbacks = device->vulkan_allocator,
	    .pVulkanFunctions     = &vulkan_functions,
	    .vulkanApiVersion     = backend->api_version,
	};

	VK_ASSERT( vmaCreateAllocator( &vma_allocator_create_info,
	                               &device->memory_allocator ) );

	enum
	{
		POOL_SIZE_COUNT = 11
	};

	VkDescriptorPoolSize pool_sizes[ POOL_SIZE_COUNT ] = {
	    { VK_DESCRIPTOR_TYPE_SAMPLER, 1024 },
	    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024 },
	    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1024 },
	    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1024 },
	    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1024 },
	    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1024 },
	    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024 },
	    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1024 },
	    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1024 },
	    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1024 },
	    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1024 },
	};

	VkDescriptorPoolCreateInfo descriptor_pool_create_info = {
	    .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
	    .pNext         = NULL,
	    .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
	    .maxSets       = 2048 * POOL_SIZE_COUNT,
	    .poolSizeCount = POOL_SIZE_COUNT,
	    .pPoolSizes    = pool_sizes,
	};

	VK_ASSERT( vkCreateDescriptorPool( device->logical_device,
	                                   &descriptor_pool_create_info,
	                                   device->vulkan_allocator,
	                                   &device->descriptor_pool ) );

	vk_pass_hasher_init( device );
}

static void
vk_destroy_device( struct ft_device* idevice )
{
	FT_FROM_HANDLE( device, idevice, vk_device );

	vk_pass_hasher_shutdown();

	vkDestroyDescriptorPool( device->logical_device,
	                         device->descriptor_pool,
	                         device->vulkan_allocator );
	vmaDestroyAllocator( device->memory_allocator );
	vkDestroyDevice( device->logical_device, device->vulkan_allocator );

	free( device );
}

static void
vk_create_queue( const struct ft_device*     idevice,
                 const struct ft_queue_info* info,
                 struct ft_queue**           p )
{
	FT_FROM_HANDLE( device, idevice, vk_device );

	FT_INIT_INTERNAL( queue, *p, vk_queue );

	uint32_t index =
	    find_queue_family_index( device->physical_device, info->queue_type );

	queue->interface.family_index = index;
	queue->interface.type         = info->queue_type;
	vkGetDeviceQueue( device->logical_device, index, 0, &queue->queue );
}

static void
vk_destroy_queue( struct ft_queue* iqueue )
{
	FT_FROM_HANDLE( queue, iqueue, vk_queue );
	free( queue );
}

static void
vk_queue_wait_idle( const struct ft_queue* iqueue )
{
	FT_FROM_HANDLE( queue, iqueue, vk_queue );
	vkQueueWaitIdle( queue->queue );
}

static void
vk_queue_submit( const struct ft_queue*             iqueue,
                 const struct ft_queue_submit_info* info )
{
	FT_FROM_HANDLE( queue, iqueue, vk_queue );

	VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;

	FT_ALLOC_STACK_ARRAY( VkSemaphore,
	                      wait_semaphores,
	                      info->wait_semaphore_count );
	FT_ALLOC_STACK_ARRAY( VkCommandBuffer,
	                      command_buffers,
	                      info->command_buffer_count );
	FT_ALLOC_STACK_ARRAY( VkSemaphore,
	                      signal_semaphores,
	                      info->signal_semaphore_count );

	for ( uint32_t i = 0; i < info->wait_semaphore_count; ++i )
	{
		FT_FROM_HANDLE( semaphore, info->wait_semaphores[ i ], vk_semaphore );

		wait_semaphores[ i ] = semaphore->semaphore;
	}

	for ( uint32_t i = 0; i < info->command_buffer_count; ++i )
	{
		FT_FROM_HANDLE( cmd, info->command_buffers[ i ], vk_command_buffer );
		command_buffers[ i ] = cmd->command_buffer;
	}

	for ( uint32_t i = 0; i < info->signal_semaphore_count; ++i )
	{
		FT_FROM_HANDLE( semaphore, info->signal_semaphores[ i ], vk_semaphore );
		signal_semaphores[ i ] = semaphore->semaphore;
	}

	VkSubmitInfo submit_info = {
	    .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
	    .pNext                = NULL,
	    .waitSemaphoreCount   = info->wait_semaphore_count,
	    .pWaitSemaphores      = wait_semaphores,
	    .pWaitDstStageMask    = &wait_dst_stage_mask,
	    .commandBufferCount   = info->command_buffer_count,
	    .pCommandBuffers      = command_buffers,
	    .signalSemaphoreCount = info->signal_semaphore_count,
	    .pSignalSemaphores    = signal_semaphores,
	};

	vkQueueSubmit(
	    queue->queue,
	    1,
	    &submit_info,
	    info->signal_fence
	        ? ( ( struct vk_fence* ) ( info->signal_fence->handle ) )->fence
	        : VK_NULL_HANDLE );
}

static void
vk_immediate_submit( const struct ft_queue*    iqueue,
                     struct ft_command_buffer* cmd )
{
	struct ft_queue_submit_info queue_submit_info = {
	    .command_buffer_count = 1,
	    .command_buffers      = &cmd,
	};

	ft_queue_submit( iqueue, &queue_submit_info );
	ft_queue_wait_idle( iqueue );
}

static void
vk_queue_present( const struct ft_queue*              iqueue,
                  const struct ft_queue_present_info* info )
{
	FT_FROM_HANDLE( swapchain, info->swapchain, vk_swapchain );
	FT_FROM_HANDLE( queue, iqueue, vk_queue );

	FT_ALLOC_STACK_ARRAY( VkSemaphore,
	                      wait_semaphores,
	                      info->wait_semaphore_count );

	for ( uint32_t i = 0; i < info->wait_semaphore_count; ++i )
	{
		FT_FROM_HANDLE( semaphore, info->wait_semaphores[ i ], vk_semaphore );
		wait_semaphores[ i ] = semaphore->semaphore;
	}

	VkPresentInfoKHR present_info = {
	    .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
	    .pNext              = NULL,
	    .waitSemaphoreCount = info->wait_semaphore_count,
	    .pWaitSemaphores    = wait_semaphores,
	    .swapchainCount     = 1,
	    .pSwapchains        = &swapchain->swapchain,
	    .pImageIndices      = &info->image_index,
	    .pResults           = NULL,
	};
	vkQueuePresentKHR( queue->queue, &present_info );
}

static void
vk_create_semaphore( const struct ft_device* idevice, struct ft_semaphore** p )
{
	FT_ASSERT( p );

	FT_FROM_HANDLE( device, idevice, vk_device );

	FT_INIT_INTERNAL( semaphore, *p, vk_semaphore );

	VkSemaphoreCreateInfo semaphore_create_info = {
	    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	};
	VK_ASSERT( vkCreateSemaphore( device->logical_device,
	                              &semaphore_create_info,
	                              device->vulkan_allocator,
	                              &semaphore->semaphore ) );
}

static void
vk_destroy_semaphore( const struct ft_device* idevice,
                      struct ft_semaphore*    isemaphore )
{
	FT_ASSERT( isemaphore );

	FT_FROM_HANDLE( device, idevice, vk_device );
	FT_FROM_HANDLE( semaphore, isemaphore, vk_semaphore );

	vkDestroySemaphore( device->logical_device,
	                    semaphore->semaphore,
	                    device->vulkan_allocator );
	free( semaphore );
}

static void
vk_create_fence( const struct ft_device* idevice, struct ft_fence** p )
{
	FT_FROM_HANDLE( device, idevice, vk_device );

	FT_INIT_INTERNAL( fence, *p, vk_fence );

	VkFenceCreateInfo fence_create_info = {
	    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
	    .pNext = NULL,
	    .flags = VK_FENCE_CREATE_SIGNALED_BIT,
	};
	VK_ASSERT( vkCreateFence( device->logical_device,
	                          &fence_create_info,
	                          device->vulkan_allocator,
	                          &fence->fence ) );
}

static void
vk_destroy_fence( const struct ft_device* idevice, struct ft_fence* ifence )
{
	FT_FROM_HANDLE( device, idevice, vk_device );
	FT_FROM_HANDLE( fence, ifence, vk_fence );

	vkDestroyFence( device->logical_device,
	                fence->fence,
	                device->vulkan_allocator );
	free( fence );
}

static void
vk_wait_for_fences( const struct ft_device* idevice,
                    uint32_t                count,
                    struct ft_fence**       ifences )
{
	FT_FROM_HANDLE( device, idevice, vk_device );

	FT_ALLOC_STACK_ARRAY( VkFence, fences, count );

	for ( uint32_t i = 0; i < count; ++i )
	{
		FT_FROM_HANDLE( fence, ifences[ i ], vk_fence );
		fences[ i ] = fence->fence;
	}

	vkWaitForFences( device->logical_device, count, fences, 1, UINT64_MAX );
}

static void
vk_reset_fences( const struct ft_device* idevice,
                 uint32_t                count,
                 struct ft_fence**       ifences )
{
	FT_FROM_HANDLE( device, idevice, vk_device );

	FT_ALLOC_STACK_ARRAY( VkFence, fences, count );

	for ( uint32_t i = 0; i < count; ++i )
	{
		FT_FROM_HANDLE( fence, ifences[ i ], vk_fence );
		fences[ i ] = fence->fence;
	}

	vkResetFences( device->logical_device, count, fences );
}

static void
vk_configure_swapchain( const struct vk_device*         device,
                        struct vk_swapchain*            swapchain,
                        const struct ft_swapchain_info* info )
{
	struct ft_wsi_info* wsi = info->wsi_info;

	wsi->create_vulkan_surface( wsi->window,
	                            device->instance,
	                            ( void** ) ( &swapchain->surface ) );

	VkBool32 support_surface = 0;
	vkGetPhysicalDeviceSurfaceSupportKHR( device->physical_device,
	                                      info->queue->family_index,
	                                      swapchain->surface,
	                                      &support_surface );

	FT_ASSERT( support_surface );

	// find best present mode
	uint32_t present_mode_count = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR( device->physical_device,
	                                           swapchain->surface,
	                                           &present_mode_count,
	                                           NULL );
	FT_ALLOC_STACK_ARRAY( VkPresentModeKHR, present_modes, present_mode_count );
	vkGetPhysicalDeviceSurfacePresentModesKHR( device->physical_device,
	                                           swapchain->surface,
	                                           &present_mode_count,
	                                           present_modes );

	swapchain->present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;

	VkPresentModeKHR preffered_mode =
	    info->vsync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_MAILBOX_KHR;

	for ( uint32_t i = 0; i < present_mode_count; ++i )
	{
		if ( present_modes[ i ] == preffered_mode )
		{
			swapchain->present_mode = preffered_mode;
			break;
		}
	}

	//	FT_INFO( "Swapchain present mode: %s",
	//	         string_VkPresentModeKHR( swapchain->present_mode ) );

	// determine present image count
	VkSurfaceCapabilitiesKHR surface_capabilities = { 0 };
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device->physical_device,
	                                           swapchain->surface,
	                                           &surface_capabilities );

	// determine swapchain size
	swapchain->interface.width =
	    FT_CLAMP( info->width,
	              surface_capabilities.minImageExtent.width,
	              surface_capabilities.maxImageExtent.width );
	swapchain->interface.height =
	    FT_CLAMP( info->height,
	              surface_capabilities.minImageExtent.height,
	              surface_capabilities.maxImageExtent.height );

	swapchain->interface.min_image_count =
	    FT_CLAMP( info->min_image_count,
	              surface_capabilities.minImageCount,
	              surface_capabilities.maxImageCount );

	/// find best surface format
	uint32_t surface_format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR( device->physical_device,
	                                      swapchain->surface,
	                                      &surface_format_count,
	                                      NULL );
	FT_ALLOC_STACK_ARRAY( VkSurfaceFormatKHR,
	                      surface_formats,
	                      surface_format_count );
	vkGetPhysicalDeviceSurfaceFormatsKHR( device->physical_device,
	                                      swapchain->surface,
	                                      &surface_format_count,
	                                      surface_formats );

	VkSurfaceFormatKHR surface_format   = surface_formats[ 0 ];
	VkFormat           preffered_format = to_vk_format( info->format );

	for ( uint32_t i = 0; i < surface_format_count; ++i )
	{
		if ( surface_formats[ i ].format == preffered_format )
		{
			surface_format = surface_formats[ i ];
			break;
		}
	}

	swapchain->interface.queue  = info->queue;
	swapchain->interface.format = from_vk_format( surface_format.format );
	swapchain->color_space      = surface_format.colorSpace;

	VkSurfaceTransformFlagBitsKHR pre_transform;
	if ( surface_capabilities.supportedTransforms &
	     VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR )
	{
		pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else
	{
		pre_transform = surface_capabilities.currentTransform;
	}
	swapchain->pre_transform   = pre_transform;
	swapchain->interface.vsync = info->vsync;
}

static void
vk_create_configured_swapchain( const struct vk_device* device,
                                struct vk_swapchain*    swapchain,
                                bool                    resize )
{
	// destroy old resources if it is resize
	if ( resize )
	{
		for ( uint32_t i = 0; i < swapchain->interface.image_count; ++i )
		{
			FT_FROM_HANDLE( image, swapchain->interface.images[ i ], vk_image );
			vkDestroyImageView( device->logical_device,
			                    image->sampled_view,
			                    device->vulkan_allocator );
			free( image );
		}
	}

	VkSwapchainCreateInfoKHR swapchain_create_info = {
	    .sType              = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
	    .pNext              = NULL,
	    .flags              = 0,
	    .surface            = swapchain->surface,
	    .minImageCount      = swapchain->interface.min_image_count,
	    .imageFormat        = to_vk_format( swapchain->interface.format ),
	    .imageColorSpace    = swapchain->color_space,
	    .imageExtent.width  = swapchain->interface.width,
	    .imageExtent.height = swapchain->interface.height,
	    .imageArrayLayers   = 1,
	    .imageUsage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
	                  VK_IMAGE_USAGE_TRANSFER_DST_BIT,
	    .imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE,
	    .queueFamilyIndexCount = 1,
	    .pQueueFamilyIndices   = &swapchain->interface.queue->family_index,
	    .preTransform          = swapchain->pre_transform,
	    // TODO: choose composite alpha according to caps
	    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
	    .presentMode    = swapchain->present_mode,
	    .clipped        = 1,
	    .oldSwapchain   = NULL,
	};

	VK_ASSERT( vkCreateSwapchainKHR( device->logical_device,
	                                 &swapchain_create_info,
	                                 device->vulkan_allocator,
	                                 &swapchain->swapchain ) );

	vkGetSwapchainImagesKHR( device->logical_device,
	                         swapchain->swapchain,
	                         &swapchain->interface.image_count,
	                         NULL );
	FT_ALLOC_STACK_ARRAY( VkImage,
	                      swapchain_images,
	                      swapchain->interface.image_count );
	vkGetSwapchainImagesKHR( device->logical_device,
	                         swapchain->swapchain,
	                         &swapchain->interface.image_count,
	                         swapchain_images );

	if ( !resize )
	{
		swapchain->interface.images = calloc( swapchain->interface.image_count,
		                                      sizeof( struct ft_image* ) );
	}

	VkImageViewCreateInfo image_view_create_info = {
	    .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
	    .pNext    = NULL,
	    .flags    = 0,
	    .viewType = VK_IMAGE_VIEW_TYPE_2D,
	    .format   = to_vk_format( swapchain->interface.format ),
	    .components =
	        {
	            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
	            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
	            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
	            .a = VK_COMPONENT_SWIZZLE_IDENTITY,
	        },
	    .subresourceRange =
	        {
	            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
	            .baseMipLevel   = 0,
	            .levelCount     = 1,
	            .baseArrayLayer = 0,
	            .layerCount     = 1,
	        },
	};

	for ( uint32_t i = 0; i < swapchain->interface.image_count; ++i )
	{
		FT_INIT_INTERNAL( image, swapchain->interface.images[ i ], vk_image );

		image_view_create_info.image = swapchain_images[ i ];

		image->image                     = swapchain_images[ i ];
		image->interface.width           = swapchain->interface.width;
		image->interface.height          = swapchain->interface.height;
		image->interface.format          = swapchain->interface.format;
		image->interface.sample_count    = 1;
		image->interface.mip_level_count = 1;
		image->interface.layer_count     = 1;
		image->interface.descriptor_type = FT_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		VK_ASSERT( vkCreateImageView( device->logical_device,
		                              &image_view_create_info,
		                              device->vulkan_allocator,
		                              &image->sampled_view ) );
	}
}

static void
vk_create_swapchain( const struct ft_device*         idevice,
                     const struct ft_swapchain_info* info,
                     struct ft_swapchain**           p )
{
	FT_ASSERT( info->wsi_info->create_vulkan_surface );

	FT_FROM_HANDLE( device, idevice, vk_device );

	FT_INIT_INTERNAL( swapchain, *p, vk_swapchain );

	vk_configure_swapchain( device, swapchain, info );
	vk_create_configured_swapchain( device, swapchain, 0 );
}

static void
vk_resize_swapchain( const struct ft_device* idevice,
                     struct ft_swapchain*    iswapchain,
                     uint32_t                width,
                     uint32_t                height )
{
	FT_FROM_HANDLE( device, idevice, vk_device );
	FT_FROM_HANDLE( swapchain, iswapchain, vk_swapchain );

	iswapchain->width  = width;
	iswapchain->height = height;

	vk_pass_hasher_framebuffers_clear();

	vk_create_configured_swapchain( device, swapchain, 1 );
}

static void
vk_destroy_swapchain( const struct ft_device* idevice,
                      struct ft_swapchain*    iswapchain )
{
	FT_FROM_HANDLE( device, idevice, vk_device );
	FT_FROM_HANDLE( swapchain, iswapchain, vk_swapchain );

	for ( uint32_t i = 0; i < swapchain->interface.image_count; ++i )
	{
		FT_FROM_HANDLE( image, swapchain->interface.images[ i ], vk_image );

		vkDestroyImageView( device->logical_device,
		                    image->sampled_view,
		                    device->vulkan_allocator );
		free( image );
	}

	free( swapchain->interface.images );

	vkDestroySwapchainKHR( device->logical_device,
	                       swapchain->swapchain,
	                       device->vulkan_allocator );

	vkDestroySurfaceKHR( device->instance,
	                     swapchain->surface,
	                     device->vulkan_allocator );

	free( swapchain );
}

static void
vk_create_command_pool( const struct ft_device*            idevice,
                        const struct ft_command_pool_info* info,
                        struct ft_command_pool**           p )
{
	FT_FROM_HANDLE( device, idevice, vk_device );

	FT_INIT_INTERNAL( command_pool, *p, vk_command_pool );

	command_pool->interface.queue = info->queue;

	VkCommandPoolCreateInfo command_pool_create_info = {
	    .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
	    .pNext            = NULL,
	    .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
	    .queueFamilyIndex = info->queue->family_index,
	};

	VK_ASSERT( vkCreateCommandPool( device->logical_device,
	                                &command_pool_create_info,
	                                device->vulkan_allocator,
	                                &command_pool->command_pool ) );
}

static void
vk_destroy_command_pool( const struct ft_device* idevice,
                         struct ft_command_pool* icommand_pool )
{
	FT_FROM_HANDLE( device, idevice, vk_device );
	FT_FROM_HANDLE( command_pool, icommand_pool, vk_command_pool );

	vkDestroyCommandPool( device->logical_device,
	                      command_pool->command_pool,
	                      device->vulkan_allocator );
	free( command_pool );
}

static void
vk_create_command_buffers( const struct ft_device*       idevice,
                           const struct ft_command_pool* icommand_pool,
                           uint32_t                      count,
                           struct ft_command_buffer**    icommand_buffers )
{
	FT_FROM_HANDLE( device, idevice, vk_device );
	FT_FROM_HANDLE( command_pool, icommand_pool, vk_command_pool );

	FT_ALLOC_STACK_ARRAY( VkCommandBuffer, buffers, count );

	VkCommandBufferAllocateInfo command_buffer_allocate_info = {
	    .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
	    .pNext              = NULL,
	    .commandPool        = command_pool->command_pool,
	    .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	    .commandBufferCount = count,
	};

	VK_ASSERT( vkAllocateCommandBuffers( device->logical_device,
	                                     &command_buffer_allocate_info,
	                                     buffers ) );

	// TODO: FIX ME!
	for ( uint32_t i = 0; i < count; ++i )
	{
		FT_INIT_INTERNAL( cmd, icommand_buffers[ i ], vk_command_buffer );

		cmd->command_buffer  = buffers[ i ];
		cmd->interface.queue = command_pool->interface.queue;
	}
}

static void
vk_free_command_buffers( const struct ft_device*       idevice,
                         const struct ft_command_pool* icommand_pool,
                         uint32_t                      count,
                         struct ft_command_buffer**    icommand_buffers )
{
	FT_FROM_HANDLE( device, idevice, vk_device );

	FT_ALLOC_STACK_ARRAY( VkCommandBuffer, buffers, count );

	for ( uint32_t i = 0; i < count; ++i )
	{
		FT_FROM_HANDLE( cmd, icommand_buffers[ i ], vk_command_buffer );
		buffers[ i ] = cmd->command_buffer;
	}

	vkFreeCommandBuffers(
	    device->logical_device,
	    ( ( struct vk_command_pool* ) ( icommand_pool->handle ) )->command_pool,
	    count,
	    buffers );
}

static void
vk_destroy_command_buffers( const struct ft_device*       idevice,
                            const struct ft_command_pool* icommand_pool,
                            uint32_t                      count,
                            struct ft_command_buffer**    icommand_buffers )
{
	( void ) idevice;
	( void ) icommand_pool;

	for ( uint32_t i = 0; i < count; ++i )
	{
		FT_FROM_HANDLE( cmd, icommand_buffers[ i ], vk_command_buffer );
		free( cmd );
	}
}

static void
vk_begin_command_buffer( const struct ft_command_buffer* icmd )
{
	FT_FROM_HANDLE( cmd, icmd, vk_command_buffer );

	VkCommandBufferBeginInfo command_buffer_begin_info = {
	    .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	    .pNext            = NULL,
	    .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	    .pInheritanceInfo = NULL,
	};

	VK_ASSERT( vkBeginCommandBuffer( cmd->command_buffer,
	                                 &command_buffer_begin_info ) );
}

static void
vk_end_command_buffer( const struct ft_command_buffer* icmd )
{
	FT_FROM_HANDLE( cmd, icmd, vk_command_buffer );
	VK_ASSERT( vkEndCommandBuffer( cmd->command_buffer ) );
}

static void
vk_acquire_next_image( const struct ft_device*    idevice,
                       const struct ft_swapchain* iswapchain,
                       const struct ft_semaphore* isemaphore,
                       const struct ft_fence*     ifence,
                       uint32_t*                  image_index )
{
	FT_FROM_HANDLE( device, idevice, vk_device );
	FT_FROM_HANDLE( swapchain, iswapchain, vk_swapchain );
	FT_FROM_HANDLE( semaphore, isemaphore, vk_semaphore );

	VkResult result = vkAcquireNextImageKHR(
	    device->logical_device,
	    swapchain->swapchain,
	    UINT64_MAX,
	    semaphore->semaphore,
	    ifence ? ( ( struct vk_fence* ) ifence->handle )->fence
	           : VK_NULL_HANDLE,
	    image_index );
	// TODO: check status
	( void ) result;
}

FT_INLINE void
vk_create_module( const struct vk_device*             device,
                  struct vk_shader*                   shader,
                  enum ft_shader_stage                stage,
                  const struct ft_shader_module_info* info )
{
	if ( info->bytecode )
	{
		VkShaderModuleCreateInfo shader_create_info = {
		    .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		    .pNext    = NULL,
		    .flags    = 0,
		    .codeSize = info->bytecode_size,
		    .pCode    = ( const uint32_t* ) ( info->bytecode ),
		};
		VK_ASSERT( vkCreateShaderModule( device->logical_device,
		                                 &shader_create_info,
		                                 device->vulkan_allocator,
		                                 &shader->shaders[ stage ] ) );
	}
}

static void
vk_create_shader( const struct ft_device* idevice,
                  struct ft_shader_info*  info,
                  struct ft_shader**      p )
{
	FT_FROM_HANDLE( device, idevice, vk_device );

	FT_INIT_INTERNAL( shader, *p, vk_shader );

	vk_create_module( device, shader, FT_SHADER_STAGE_COMPUTE, &info->compute );
	vk_create_module( device, shader, FT_SHADER_STAGE_VERTEX, &info->vertex );
	vk_create_module( device,
	                  shader,
	                  FT_SHADER_STAGE_TESSELLATION_CONTROL,
	                  &info->tessellation_control );
	vk_create_module( device,
	                  shader,
	                  FT_SHADER_STAGE_TESSELLATION_EVALUATION,
	                  &info->tessellation_evaluation );
	vk_create_module( device,
	                  shader,
	                  FT_SHADER_STAGE_GEOMETRY,
	                  &info->geometry );
	vk_create_module( device,
	                  shader,
	                  FT_SHADER_STAGE_FRAGMENT,
	                  &info->fragment );

	spirv_reflect( idevice, info, &shader->interface );
}

FT_INLINE void
vk_destroy_module( const struct vk_device* device,
                   enum ft_shader_stage    stage,
                   struct vk_shader*       shader )
{
	if ( shader->shaders[ stage ] )
	{
		vkDestroyShaderModule( device->logical_device,
		                       shader->shaders[ stage ],
		                       device->vulkan_allocator );
	}
}

static void
vk_destroy_shader( const struct ft_device* idevice, struct ft_shader* ishader )
{
	FT_FROM_HANDLE( device, idevice, vk_device );
	FT_FROM_HANDLE( shader, ishader, vk_shader );

	vk_destroy_module( device, FT_SHADER_STAGE_COMPUTE, shader );
	vk_destroy_module( device, FT_SHADER_STAGE_VERTEX, shader );
	vk_destroy_module( device, FT_SHADER_STAGE_TESSELLATION_CONTROL, shader );
	vk_destroy_module( device,
	                   FT_SHADER_STAGE_TESSELLATION_EVALUATION,
	                   shader );
	vk_destroy_module( device, FT_SHADER_STAGE_GEOMETRY, shader );
	vk_destroy_module( device, FT_SHADER_STAGE_FRAGMENT, shader );

	hashmap_free( shader->interface.reflect_data.binding_map );

	if ( shader->interface.reflect_data.binding_count )
	{
		free( shader->interface.reflect_data.bindings );
	}

	free( shader );
}

static void
vk_create_descriptor_set_layout( const struct ft_device*           idevice,
                                 struct ft_shader*                 ishader,
                                 struct ft_descriptor_set_layout** p )
{
	FT_FROM_HANDLE( device, idevice, vk_device );

	FT_INIT_INTERNAL( descriptor_set_layout, *p, vk_descriptor_set_layout );

	// copy reflection data because shader can be destroyed earlier
	struct ft_reflection_data reflect_data = {
	    .binding_count = ishader->reflect_data.binding_count,
	    .binding_map   = hashmap_new( sizeof( struct ft_binding_map_item ),
                                    0,
                                    0,
                                    0,
                                    binding_map_hash,
                                    binding_map_compare,
                                    NULL,
                                    NULL ),
	};

	if ( reflect_data.binding_count != 0 )
	{
		size_t iter = 0;
		void*  item;
		while (
		    hashmap_iter( ishader->reflect_data.binding_map, &iter, &item ) )
		{
			hashmap_set( reflect_data.binding_map, item );
		}

		FT_ALLOC_HEAP_ARRAY( struct ft_binding,
		                     bindings,
		                     ishader->reflect_data.binding_count );
		reflect_data.bindings = bindings;
		for ( uint32_t i = 0; i < ishader->reflect_data.binding_count; ++i )
		{
			reflect_data.bindings[ i ] = ishader->reflect_data.bindings[ i ];
		}
	}
	descriptor_set_layout->interface.reflection_data = reflect_data;

	// count bindings in all shaders
	uint32_t binding_counts[ FT_MAX_SET_COUNT ] = { 0 };
	VkDescriptorBindingFlags
	    binding_flags[ FT_MAX_SET_COUNT ][ FT_MAX_DESCRIPTOR_BINDING_COUNT ] = {
	        0 };
	// collect all bindings
	VkDescriptorSetLayoutBinding bindings[ FT_MAX_SET_COUNT ]
	                                     [ FT_MAX_DESCRIPTOR_BINDING_COUNT ];

	uint32_t set_count = 0;

	struct ft_reflection_data* reflection =
	    &descriptor_set_layout->interface.reflection_data;

	for ( uint32_t b = 0; b < reflection->binding_count; ++b )
	{
		struct ft_binding* binding       = &reflection->bindings[ b ];
		uint32_t           set           = binding->set;
		uint32_t           binding_count = binding_counts[ set ];

		bindings[ set ][ binding_count ].binding = binding->binding;
		bindings[ set ][ binding_count ].descriptorCount =
		    binding->descriptor_count;
		bindings[ set ][ binding_count ].descriptorType =
		    to_vk_descriptor_type( binding->descriptor_type );
		bindings[ set ][ binding_count ].pImmutableSamplers = NULL; // ??? TODO
		bindings[ set ][ binding_count ].stageFlags =
		    to_vk_shader_stage( binding->stage );

		if ( binding->descriptor_count > 1 )
		{
			VkDescriptorBindingFlags flags = ( VkDescriptorBindingFlags ) 0;
			binding_flags[ set ][ binding_count ] = flags;
			binding_flags[ set ][ binding_count ] |=
			    VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
		}

		binding_count++;
		binding_counts[ set ] = binding_count;

		if ( set + 1 > set_count )
		{
			set_count = set + 1;
		}
	}

	for ( uint32_t set = 0; set < set_count; ++set )
	{
		if ( binding_counts[ set ] > 0 )
		{
			VkDescriptorSetLayoutBindingFlagsCreateInfo
			    binding_flags_create_info = { 0 };
			binding_flags_create_info.sType =
			    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
			binding_flags_create_info.pNext         = NULL;
			binding_flags_create_info.bindingCount  = binding_counts[ set ];
			binding_flags_create_info.pBindingFlags = binding_flags[ set ];

			VkDescriptorSetLayoutCreateInfo dsl_create_info = {
			    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			    .pNext = &binding_flags_create_info,
			    .flags = 0,
			    .bindingCount = binding_counts[ set ],
			    .pBindings    = bindings[ set ],
			};
			VK_ASSERT( vkCreateDescriptorSetLayout(
			    device->logical_device,
			    &dsl_create_info,
			    device->vulkan_allocator,
			    &descriptor_set_layout->descriptor_set_layouts[ set ] ) );

			descriptor_set_layout->descriptor_set_layout_count++;
		}
	}
}

static void
vk_destroy_descriptor_set_layout( const struct ft_device*          idevice,
                                  struct ft_descriptor_set_layout* ilayout )
{
	FT_FROM_HANDLE( device, idevice, vk_device );
	FT_FROM_HANDLE( layout, ilayout, vk_descriptor_set_layout );

	for ( uint32_t i = 0; i < layout->descriptor_set_layout_count; ++i )
	{
		if ( layout->descriptor_set_layouts[ i ] )
		{
			vkDestroyDescriptorSetLayout( device->logical_device,
			                              layout->descriptor_set_layouts[ i ],
			                              device->vulkan_allocator );
		}
	}

	hashmap_free( layout->interface.reflection_data.binding_map );

	if ( layout->interface.reflection_data.binding_count )
	{
		free( layout->interface.reflection_data.bindings );
	}

	free( layout );
}

FT_INLINE void
vk_create_compute_pipeline( const struct ft_device*        idevice,
                            const struct ft_pipeline_info* info,
                            struct ft_pipeline**           p )
{
	FT_FROM_HANDLE( device, idevice, vk_device );
	FT_FROM_HANDLE( shader, info->shader, vk_shader );
	FT_FROM_HANDLE( dsl,
	                info->descriptor_set_layout,
	                vk_descriptor_set_layout );

	FT_INIT_INTERNAL( pipeline, *p, vk_pipeline );

	pipeline->interface.type = FT_PIPELINE_TYPE_COMPUTE;

	VkPipelineShaderStageCreateInfo shader_stage_create_info = {
	    .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	    .pNext  = NULL,
	    .flags  = 0,
	    .stage  = VK_SHADER_STAGE_COMPUTE_BIT,
	    .module = shader->shaders[ FT_SHADER_STAGE_COMPUTE ],
	    .pName  = "main",
	    .pSpecializationInfo = NULL,
	};

	VkPushConstantRange push_constant_range = {
	    .size       = FT_MAX_PUSH_CONSTANT_RANGE,
	    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT |
	                  VK_SHADER_STAGE_FRAGMENT_BIT,
	};

	VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
	    .sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
	    .setLayoutCount = dsl->descriptor_set_layout_count,
	    .pSetLayouts    = ( ( struct vk_descriptor_set_layout* )
                             info->descriptor_set_layout->handle )
	                       ->descriptor_set_layouts,
	    .pushConstantRangeCount = 1,
	    .pPushConstantRanges    = &push_constant_range,
	};

	VK_ASSERT( vkCreatePipelineLayout( device->logical_device,
	                                   &pipeline_layout_create_info,
	                                   NULL,
	                                   &pipeline->pipeline_layout ) );

	VkComputePipelineCreateInfo compute_pipeline_create_info = {
	    .sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
	    .stage  = shader_stage_create_info,
	    .layout = pipeline->pipeline_layout,
	};
	VK_ASSERT( vkCreateComputePipelines( device->logical_device,
	                                     VK_NULL_HANDLE,
	                                     1,
	                                     &compute_pipeline_create_info,
	                                     device->vulkan_allocator,
	                                     &pipeline->pipeline ) );
}

FT_INLINE void
vk_create_graphics_pipeline( const struct ft_device*        idevice,
                             const struct ft_pipeline_info* info,
                             struct ft_pipeline**           p )
{
	FT_FROM_HANDLE( device, idevice, vk_device );
	FT_FROM_HANDLE( shader, info->shader, vk_shader );
	FT_FROM_HANDLE( dsl,
	                info->descriptor_set_layout,
	                vk_descriptor_set_layout );

	FT_INIT_INTERNAL( pipeline, *p, vk_pipeline );

	struct ft_render_pass_begin_info render_pass_info = { 0 };
	render_pass_info.color_attachment_count = info->color_attachment_count;
	FT_ALLOC_STACK_ARRAY( struct ft_image,
	                      color_images,
	                      info->color_attachment_count );
	for ( uint32_t i = 0; i < info->color_attachment_count; ++i )
	{
		color_images[ i ].format       = info->color_attachment_formats[ i ];
		color_images[ i ].sample_count = info->sample_count;
		render_pass_info.color_attachments[ i ].image = &color_images[ i ];
		render_pass_info.color_attachments[ i ].load_op =
		    FT_ATTACHMENT_LOAD_OP_DONT_CARE;
	}

	struct ft_image depth_image;
	if ( info->depth_stencil_format != FT_FORMAT_UNDEFINED )
	{
		depth_image.format                      = info->depth_stencil_format;
		depth_image.sample_count                = info->sample_count;
		render_pass_info.depth_attachment.image = &depth_image;
		render_pass_info.depth_attachment.load_op =
		    FT_ATTACHMENT_LOAD_OP_DONT_CARE;
	}

	VkRenderPass render_pass;
	vk_create_render_pass( device, &render_pass_info, &render_pass );

	pipeline->interface.type = FT_PIPELINE_TYPE_GRAPHICS;

	uint32_t shader_stage_count = 0;
	VkPipelineShaderStageCreateInfo
	    shader_stage_create_infos[ FT_SHADER_STAGE_COUNT ];

	for ( uint32_t i = 0; i < FT_SHADER_STAGE_COUNT; ++i )
	{
		if ( shader->shaders[ i ] == VK_NULL_HANDLE )
		{
			continue;
		}

		shader_stage_create_infos[ shader_stage_count ].sType =
		    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stage_create_infos[ shader_stage_count ].pNext = NULL;
		shader_stage_create_infos[ shader_stage_count ].flags = 0;
		shader_stage_create_infos[ shader_stage_count ].stage =
		    to_vk_shader_stage( i );
		shader_stage_create_infos[ shader_stage_count ].module =
		    shader->shaders[ i ];
		shader_stage_create_infos[ shader_stage_count ].pName = "main";
		shader_stage_create_infos[ shader_stage_count ].pSpecializationInfo =
		    NULL;
		shader_stage_count++;
	}

	FT_ASSERT( shader_stage_count > 0 );

	const struct ft_vertex_layout* vertex_layout = &info->vertex_layout;

	VkVertexInputBindingDescription
	    binding_descriptions[ FT_MAX_VERTEX_BINDING_COUNT ];
	for ( uint32_t i = 0; i < vertex_layout->binding_info_count; ++i )
	{
		binding_descriptions[ i ].binding =
		    vertex_layout->binding_infos[ i ].binding;
		binding_descriptions[ i ].stride =
		    vertex_layout->binding_infos[ i ].stride;
		binding_descriptions[ i ].inputRate = to_vk_vertex_input_rate(
		    vertex_layout->binding_infos[ i ].input_rate );
	}

	VkVertexInputAttributeDescription
	    attribute_descriptions[ FT_MAX_VERTEX_ATTRIBUTE_COUNT ];
	for ( uint32_t i = 0; i < vertex_layout->attribute_info_count; ++i )
	{
		attribute_descriptions[ i ].location =
		    vertex_layout->attribute_infos[ i ].location;
		attribute_descriptions[ i ].binding =
		    vertex_layout->attribute_infos[ i ].binding;
		attribute_descriptions[ i ].format =
		    to_vk_format( vertex_layout->attribute_infos[ i ].format );
		attribute_descriptions[ i ].offset =
		    vertex_layout->attribute_infos[ i ].offset;
	}

	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
	    .vertexBindingDescriptionCount   = vertex_layout->binding_info_count,
	    .pVertexBindingDescriptions      = binding_descriptions,
	    .vertexAttributeDescriptionCount = vertex_layout->attribute_info_count,
	    .pVertexAttributeDescriptions    = attribute_descriptions,
	};

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {
	    .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
	    .topology = to_vk_primitive_topology( info->topology ),
	    .primitiveRestartEnable = 0,
	};

	// Dynamic states
	VkViewport viewport = { 0 };
	VkRect2D   scissor  = { 0 };

	VkPipelineViewportStateCreateInfo viewport_state_create_info = {
	    .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
	    .viewportCount = 1,
	    .pViewports    = &viewport,
	    .scissorCount  = 1,
	    .pScissors     = &scissor,
	};

	VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
	    .polygonMode = to_vk_polygon_mode( info->rasterizer_info.polygon_mode ),
	    .cullMode    = to_vk_cull_mode( info->rasterizer_info.cull_mode ),
	    .frontFace   = to_vk_front_face( info->rasterizer_info.front_face ),
	    .lineWidth   = 1.0f,
	};

	VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
	    .rasterizationSamples = to_vk_sample_count( info->sample_count ),
	    .sampleShadingEnable  = VK_FALSE,
	    .minSampleShading     = 1.0f,
	};

	FT_ALLOC_STACK_ARRAY( VkPipelineColorBlendAttachmentState,
	                      attachment_states,
	                      info->color_attachment_count );
	const struct ft_blend_state_info* blend = &info->blend_state_info;
	for ( uint32_t i = 0; i < info->color_attachment_count; ++i )
	{
		VkPipelineColorBlendAttachmentState attachment_state = { 0 };
		attachment_states[ i ] = ( VkPipelineColorBlendAttachmentState ) {
		    .blendEnable = blend->attachment_states[ i ].src_factor !=
		                   FT_BLEND_FACTOR_ZERO,
		    .srcColorBlendFactor =
		        to_vk_blend_factor( blend->attachment_states[ i ].src_factor ),
		    .dstColorBlendFactor =
		        to_vk_blend_factor( blend->attachment_states[ i ].dst_factor ),
		    .colorBlendOp = to_vk_blend_op( blend->attachment_states[ i ].op ),
		    .srcAlphaBlendFactor = to_vk_blend_factor(
		        blend->attachment_states[ i ].src_alpha_factor ),
		    .dstAlphaBlendFactor = to_vk_blend_factor(
		        blend->attachment_states[ i ].dst_alpha_factor ),
		    .alphaBlendOp =
		        to_vk_blend_op( blend->attachment_states[ i ].alpha_op ),
		    .colorWriteMask =
		        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		};
	}

	VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
	    .attachmentCount = info->color_attachment_count,
	    .pAttachments    = attachment_states,
	};

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
	    .depthTestEnable =
	        info->depth_state_info.depth_test ? VK_TRUE : VK_FALSE,
	    .depthWriteEnable =
	        info->depth_state_info.depth_write ? VK_TRUE : VK_FALSE,
	    .depthCompareOp =
	        info->depth_state_info.depth_test
	            ? to_vk_compare_op( info->depth_state_info.compare_op )
	            : VK_COMPARE_OP_ALWAYS,
	    .depthBoundsTestEnable = VK_FALSE,
	    .minDepthBounds        = 0.0f,
	    .maxDepthBounds        = 1.0f,
	    .stencilTestEnable     = VK_FALSE,
	};

	enum
	{
		DYNAMIC_STATE_COUNT = 2
	};

	VkDynamicState dynamic_states[ DYNAMIC_STATE_COUNT ] = {
	    VK_DYNAMIC_STATE_SCISSOR,
	    VK_DYNAMIC_STATE_VIEWPORT };

	VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
	    .dynamicStateCount = DYNAMIC_STATE_COUNT,
	    .pDynamicStates    = dynamic_states,
	};

	VkPushConstantRange push_constant_range = {
	    .size       = FT_MAX_PUSH_CONSTANT_RANGE,
	    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT |
	                  VK_SHADER_STAGE_FRAGMENT_BIT,
	};

	VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
	    .sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
	    .setLayoutCount = dsl->descriptor_set_layout_count,
	    .pSetLayouts    = ( ( struct vk_descriptor_set_layout* )
                             info->descriptor_set_layout->handle )
	                       ->descriptor_set_layouts,
	    .pushConstantRangeCount = 1,
	    .pPushConstantRanges    = &push_constant_range,
	};

	VK_ASSERT( vkCreatePipelineLayout( device->logical_device,
	                                   &pipeline_layout_create_info,
	                                   device->vulkan_allocator,
	                                   &pipeline->pipeline_layout ) );

	VkGraphicsPipelineCreateInfo pipeline_create_info = {
	    .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
	    .stageCount          = shader_stage_count,
	    .pStages             = shader_stage_create_infos,
	    .pVertexInputState   = &vertex_input_state_create_info,
	    .pInputAssemblyState = &input_assembly_state_create_info,
	    .pViewportState      = &viewport_state_create_info,
	    .pMultisampleState   = &multisample_state_create_info,
	    .pRasterizationState = &rasterization_state_create_info,
	    .pColorBlendState    = &color_blend_state_create_info,
	    .pDepthStencilState  = &depth_stencil_state_create_info,
	    .pDynamicState       = &dynamic_state_create_info,
	    .layout              = pipeline->pipeline_layout,
	    .renderPass          = render_pass,
	};
	VK_ASSERT( vkCreateGraphicsPipelines( device->logical_device,
	                                      VK_NULL_HANDLE,
	                                      1,
	                                      &pipeline_create_info,
	                                      device->vulkan_allocator,
	                                      &pipeline->pipeline ) );

	vkDestroyRenderPass( device->logical_device,
	                     render_pass,
	                     device->vulkan_allocator );
}

static void
vk_create_pipeline( const struct ft_device*        idevice,
                    const struct ft_pipeline_info* info,
                    struct ft_pipeline**           p )
{
	switch ( info->type )
	{
	case FT_PIPELINE_TYPE_COMPUTE:
	{
		vk_create_compute_pipeline( idevice, info, p );
		break;
	}
	case FT_PIPELINE_TYPE_GRAPHICS:
	{
		vk_create_graphics_pipeline( idevice, info, p );
		break;
	}
	default:
	{
		FT_ASSERT( false );
		break;
	}
	}
}

static void
vk_destroy_pipeline( const struct ft_device* idevice,
                     struct ft_pipeline*     ipipeline )
{
	FT_FROM_HANDLE( device, idevice, vk_device );
	FT_FROM_HANDLE( pipeline, ipipeline, vk_pipeline );

	vkDestroyPipelineLayout( device->logical_device,
	                         pipeline->pipeline_layout,
	                         device->vulkan_allocator );
	vkDestroyPipeline( device->logical_device,
	                   pipeline->pipeline,
	                   device->vulkan_allocator );
	free( pipeline );
}

static void
vk_create_buffer( const struct ft_device*      idevice,
                  const struct ft_buffer_info* info,
                  struct ft_buffer**           p )
{
	FT_FROM_HANDLE( device, idevice, vk_device );

	FT_INIT_INTERNAL( buffer, *p, vk_buffer );

	buffer->interface.size            = info->size;
	buffer->interface.descriptor_type = info->descriptor_type;
	buffer->interface.memory_usage    = info->memory_usage;

	VmaAllocationCreateInfo allocation_create_info = {
	    .usage = determine_vma_memory_usage( info->memory_usage ),
	};

	VkBufferCreateInfo buffer_create_info = {
	    .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
	    .pNext       = NULL,
	    .flags       = 0,
	    .size        = info->size,
	    .usage       = determine_vk_buffer_usage( info->descriptor_type,
                                            info->memory_usage ),
	    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	    .queueFamilyIndexCount = 0,
	    .pQueueFamilyIndices   = NULL,
	};

	VK_ASSERT( vmaCreateBuffer( device->memory_allocator,
	                            &buffer_create_info,
	                            &allocation_create_info,
	                            &buffer->buffer,
	                            &buffer->allocation,
	                            NULL ) );
}

static void
vk_destroy_buffer( const struct ft_device* idevice, struct ft_buffer* ibuffer )
{
	FT_FROM_HANDLE( device, idevice, vk_device );
	FT_FROM_HANDLE( buffer, ibuffer, vk_buffer );

	vmaDestroyBuffer( device->memory_allocator,
	                  buffer->buffer,
	                  buffer->allocation );
	free( buffer );
}

static void
vk_create_sampler( const struct ft_device*       idevice,
                   const struct ft_sampler_info* info,
                   struct ft_sampler**           p )
{
	FT_FROM_HANDLE( device, idevice, vk_device );

	FT_INIT_INTERNAL( sampler, *p, vk_sampler );

	VkSamplerCreateInfo sampler_create_info = {
	    .sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
	    .pNext            = NULL,
	    .flags            = 0,
	    .magFilter        = to_vk_filter( info->mag_filter ),
	    .minFilter        = to_vk_filter( info->min_filter ),
	    .mipmapMode       = to_vk_sampler_mipmap_mode( info->mipmap_mode ),
	    .addressModeU     = to_vk_sampler_address_mode( info->address_mode_u ),
	    .addressModeV     = to_vk_sampler_address_mode( info->address_mode_v ),
	    .addressModeW     = to_vk_sampler_address_mode( info->address_mode_w ),
	    .mipLodBias       = info->mip_lod_bias,
	    .anisotropyEnable = info->anisotropy_enable,
	    .maxAnisotropy    = info->max_anisotropy,
	    .compareEnable    = info->compare_enable,
	    .compareOp        = to_vk_compare_op( info->compare_op ),
	    .minLod           = info->min_lod,
	    .maxLod           = info->max_lod,
	    .unnormalizedCoordinates = 0,
	};

	VK_ASSERT( vkCreateSampler( device->logical_device,
	                            &sampler_create_info,
	                            device->vulkan_allocator,
	                            &sampler->sampler ) );
}

static void
vk_destroy_sampler( const struct ft_device* idevice,
                    struct ft_sampler*      isampler )
{
	FT_FROM_HANDLE( device, idevice, vk_device );
	FT_FROM_HANDLE( sampler, isampler, vk_sampler );

	vkDestroySampler( device->logical_device,
	                  sampler->sampler,
	                  device->vulkan_allocator );
	free( sampler );
}

static void
vk_create_image( const struct ft_device*     idevice,
                 const struct ft_image_info* info,
                 struct ft_image**           p )
{
	FT_FROM_HANDLE( device, idevice, vk_device );

	FT_INIT_INTERNAL( image, *p, vk_image );

	VmaAllocationCreateInfo allocation_create_info = {
	    .usage = VMA_MEMORY_USAGE_GPU_ONLY,
	};

	VkImageCreateInfo image_create_info = {
	    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	    // TODO: determine image type properly
	    .imageType     = VK_IMAGE_TYPE_2D,
	    .format        = to_vk_format( info->format ),
	    .extent.width  = info->width,
	    .extent.height = info->height,
	    .extent.depth  = info->depth,
	    .mipLevels     = info->mip_levels,
	    .arrayLayers   = info->layer_count,
	    .samples       = to_vk_sample_count( info->sample_count ),
	    .tiling        = VK_IMAGE_TILING_OPTIMAL,
	    .usage         = determine_vk_image_usage( info->descriptor_type ),
	    .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
	    .queueFamilyIndexCount = 0,
	    .pQueueFamilyIndices   = NULL,
	    .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	if ( info->layer_count == 6 )
	{
		image_create_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}

	VK_ASSERT( vmaCreateImage( device->memory_allocator,
	                           &image_create_info,
	                           &allocation_create_info,
	                           &image->image,
	                           &image->allocation,
	                           NULL ) );

	image->interface.format          = info->format;
	image->interface.mip_level_count = info->mip_levels;
	image->interface.layer_count     = info->layer_count;

	VkImageViewCreateInfo sampled_view_create_info = {
	    .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
	    .pNext    = NULL,
	    .flags    = 0,
	    .image    = image->image,
	    .viewType = VK_IMAGE_VIEW_TYPE_2D,
	    .format   = image_create_info.format,
	    .subresourceRange =
	        {
	            .aspectMask     = get_aspect_mask( info->format ),
	            .baseArrayLayer = 0,
	            .levelCount     = info->mip_levels,
	            .layerCount     = info->layer_count,
	        },
	    .components =
	        {
	            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
	            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
	            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
	            .a = VK_COMPONENT_SWIZZLE_IDENTITY,
	        },
	};

	if ( info->layer_count > 1 )
	{
		sampled_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	}
	
	// TODO:
	if ( info->layer_count == 6 )
	{
		sampled_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	}

	VK_ASSERT( vkCreateImageView( device->logical_device,
	                              &sampled_view_create_info,
	                              device->vulkan_allocator,
	                              &image->sampled_view ) );

	if ( info->descriptor_type & FT_DESCRIPTOR_TYPE_STORAGE_IMAGE )
	{
		image->storage_views =
		    malloc( sizeof( VkImageView ) * info->mip_levels );

		VkImageViewCreateInfo storage_view_create_info =
		    sampled_view_create_info;
		storage_view_create_info.subresourceRange.levelCount = 1;

		if ( storage_view_create_info.viewType == VK_IMAGE_VIEW_TYPE_CUBE )
		{
			storage_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		}

		for ( uint32_t mip = 0; mip < info->mip_levels; ++mip )
		{
			storage_view_create_info.subresourceRange.baseMipLevel = mip;

			VK_ASSERT( vkCreateImageView( device->logical_device,
			                              &storage_view_create_info,
			                              device->vulkan_allocator,
			                              &image->storage_views[ mip ] ) );
		}
	}
}

static void
vk_destroy_image( const struct ft_device* idevice, struct ft_image* iimage )
{
	FT_FROM_HANDLE( device, idevice, vk_device );
	FT_FROM_HANDLE( image, iimage, vk_image );

	if ( image->storage_views )
	{
		for ( uint32_t mip = 0; mip < image->interface.mip_level_count; ++mip )
		{
			vkDestroyImageView( device->logical_device,
			                    image->storage_views[ mip ],
			                    device->vulkan_allocator );
		}
		free( image->storage_views );
	}

	vkDestroyImageView( device->logical_device,
	                    image->sampled_view,
	                    device->vulkan_allocator );

	vmaDestroyImage( device->memory_allocator,
	                 image->image,
	                 image->allocation );
	free( image );
}

static void
vk_create_descriptor_set( const struct ft_device*              idevice,
                          const struct ft_descriptor_set_info* info,
                          struct ft_descriptor_set**           p )
{
	FT_FROM_HANDLE( device, idevice, vk_device );

	FT_INIT_INTERNAL( descriptor_set, *p, vk_descriptor_set );

	descriptor_set->interface.layout = info->descriptor_set_layout;

	VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {
	    .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
	    .pNext              = NULL,
	    .descriptorPool     = device->descriptor_pool,
	    .descriptorSetCount = 1,
	    .pSetLayouts        = &( ( struct vk_descriptor_set_layout* )
                              info->descriptor_set_layout->handle )
	                        ->descriptor_set_layouts[ info->set ],
	};

	VK_ASSERT( vkAllocateDescriptorSets( device->logical_device,
	                                     &descriptor_set_allocate_info,
	                                     &descriptor_set->descriptor_set ) );
}

static void
vk_destroy_descriptor_set( const struct ft_device*   idevice,
                           struct ft_descriptor_set* iset )
{
	FT_FROM_HANDLE( device, idevice, vk_device );
	FT_FROM_HANDLE( set, iset, vk_descriptor_set );

	vkFreeDescriptorSets( device->logical_device,
	                      device->descriptor_pool,
	                      1,
	                      &set->descriptor_set );
	free( set );
}

static void
vk_update_descriptor_set( const struct ft_device*           idevice,
                          struct ft_descriptor_set*         iset,
                          uint32_t                          count,
                          const struct ft_descriptor_write* writes )
{
	FT_FROM_HANDLE( device, idevice, vk_device );
	FT_FROM_HANDLE( set, iset, vk_descriptor_set );

	// TODO: rewrite
	FT_ALLOC_HEAP_ARRAY( VkDescriptorBufferInfo*, buffer_updates, count );
	uint32_t buffer_update_idx = 0;
	FT_ALLOC_HEAP_ARRAY( VkDescriptorImageInfo*, image_updates, count );
	uint32_t image_update_idx = 0;
	FT_ALLOC_HEAP_ARRAY( VkWriteDescriptorSet, descriptor_writes, count );

	uint32_t write = 0;

	for ( uint32_t i = 0; i < count; ++i )
	{
		const struct ft_descriptor_write* descriptor_write = &writes[ i ];

		struct ft_reflection_data* reflection =
		    &set->interface.layout->reflection_data;

		struct ft_binding_map_item item;
		memset( item.name, '\0', FT_MAX_BINDING_NAME_LENGTH );
		strcpy( item.name, descriptor_write->descriptor_name );
		struct ft_binding_map_item* it =
		    hashmap_get( reflection->binding_map, &item );

		FT_ASSERT( it != NULL );

		if ( it == NULL )
		{
			FT_WARN( "descriptor with name %s not founded",
			         descriptor_write->descriptor_name );
			break;
		}

		const struct ft_binding* binding = &reflection->bindings[ it->value ];

		VkWriteDescriptorSet* write_descriptor_set =
		    &descriptor_writes[ write++ ];
		write_descriptor_set->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_descriptor_set->dstBinding = binding->binding;
		write_descriptor_set->descriptorCount =
		    descriptor_write->descriptor_count;
		write_descriptor_set->dstSet = set->descriptor_set;
		write_descriptor_set->descriptorType =
		    to_vk_descriptor_type( binding->descriptor_type );

		if ( descriptor_write->buffer_descriptors )
		{
			VkDescriptorBufferInfo* buffer_infos =
			    calloc( sizeof( VkDescriptorBufferInfo ),
			            descriptor_write->descriptor_count );
			buffer_updates[ buffer_update_idx++ ] = buffer_infos;

			for ( uint32_t j = 0; j < descriptor_write->descriptor_count; ++j )
			{
				buffer_infos[ j ].buffer =
				    ( ( struct vk_buffer* ) descriptor_write
				          ->buffer_descriptors[ j ]
				          .buffer->handle )
				        ->buffer;
				buffer_infos[ j ].offset =
				    descriptor_write->buffer_descriptors[ j ].offset;
				buffer_infos[ j ].range =
				    descriptor_write->buffer_descriptors[ j ].range;
			}

			write_descriptor_set->pBufferInfo = buffer_infos;
		}
		else if ( descriptor_write->image_descriptors )
		{
			VkDescriptorImageInfo* image_infos =
			    calloc( sizeof( VkDescriptorImageInfo ),
			            descriptor_write->descriptor_count );
			image_updates[ image_update_idx++ ] = image_infos;

			for ( uint32_t j = 0; j < descriptor_write->descriptor_count; ++j )
			{
				struct ft_image_descriptor* descriptor =
				    &descriptor_write->image_descriptors[ j ];

				FT_ASSERT( descriptor->image );
				FT_ASSERT( descriptor->mip_level <
				           descriptor->image->mip_level_count );

				image_infos[ j ].imageLayout =
				    determine_image_layout( descriptor->resource_state );

				struct vk_image* image = descriptor->image->handle;
				switch ( descriptor->resource_state )
				{
				case FT_RESOURCE_STATE_GENERAL:
					image_infos[ j ].imageView =
					    image->storage_views[ descriptor->mip_level ];
					break;
				default:
					image_infos[ j ].imageView = image->sampled_view;
					break;
				}

				image_infos[ j ].sampler = NULL;
			}

			write_descriptor_set->pImageInfo = image_infos;
		}
		else
		{
			VkDescriptorImageInfo* image_infos =
			    calloc( sizeof( VkDescriptorImageInfo ),
			            descriptor_write->descriptor_count );
			image_updates[ image_update_idx++ ] = image_infos;

			for ( uint32_t j = 0; j < descriptor_write->descriptor_count; ++j )
			{
				struct ft_sampler_descriptor* descriptor =
				    &descriptor_write->sampler_descriptors[ j ];

				FT_ASSERT( descriptor->sampler );

				image_infos[ j ].sampler =
				    ( ( struct vk_sampler* ) descriptor->sampler->handle )
				        ->sampler;
			}

			write_descriptor_set->pImageInfo = image_infos;
		}
	}

	vkUpdateDescriptorSets( device->logical_device,
	                        count,
	                        descriptor_writes,
	                        0,
	                        NULL );

	for ( uint32_t i = 0; i < count; ++i )
	{
		if ( buffer_updates[ i ] != NULL )
		{
			free( buffer_updates[ i ] );
		}

		if ( image_updates[ i ] != NULL )
		{
			free( image_updates[ i ] );
		}
	}

	free( buffer_updates );
	free( image_updates );
	free( descriptor_writes );
}

static void
vk_cmd_begin_render_pass( const struct ft_command_buffer*         icmd,
                          const struct ft_render_pass_begin_info* info )
{
	FT_FROM_HANDLE( cmd, icmd, vk_command_buffer );

	VkRenderPass  render_pass = vk_pass_hasher_get_render_pass( info );
	VkFramebuffer framebuffer =
	    vk_pass_hasher_get_framebuffer( render_pass, info );

	static VkClearValue clear_values[ FT_MAX_ATTACHMENTS_COUNT + 1 ];
	uint32_t            clear_value_count = info->color_attachment_count;

	for ( uint32_t i = 0; i < info->color_attachment_count; ++i )
	{
		VkClearValue clear_value = { 0 };
		clear_values[ i ]        = clear_value;
		clear_values[ i ].color.float32[ 0 ] =
		    info->color_attachments[ i ].clear_value.color[ 0 ];
		clear_values[ i ].color.float32[ 1 ] =
		    info->color_attachments[ i ].clear_value.color[ 1 ];
		clear_values[ i ].color.float32[ 2 ] =
		    info->color_attachments[ i ].clear_value.color[ 2 ];
		clear_values[ i ].color.float32[ 3 ] =
		    info->color_attachments[ i ].clear_value.color[ 3 ];
	}

	if ( info->depth_attachment.image )
	{
		clear_value_count++;
		uint32_t     idx         = info->color_attachment_count;
		VkClearValue clear_value = { 0 };
		clear_values[ idx ]      = clear_value;
		clear_values[ idx ].depthStencil.depth =
		    info->depth_attachment.clear_value.depth_stencil.depth;
		clear_values[ idx ].depthStencil.stencil =
		    info->depth_attachment.clear_value.depth_stencil.stencil;
	}

	VkRenderPassBeginInfo render_pass_begin_info = {
	    .sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
	    .pNext                    = NULL,
	    .renderPass               = render_pass,
	    .framebuffer              = framebuffer,
	    .renderArea.extent.width  = info->width,
	    .renderArea.extent.height = info->height,
	    .renderArea.offset.x      = 0,
	    .renderArea.offset.y      = 0,
	    .clearValueCount          = clear_value_count,
	    .pClearValues             = clear_values,
	};

	vkCmdBeginRenderPass( cmd->command_buffer,
	                      &render_pass_begin_info,
	                      VK_SUBPASS_CONTENTS_INLINE );
}

static void
vk_cmd_end_render_pass( const struct ft_command_buffer* icmd )
{
	FT_FROM_HANDLE( cmd, icmd, vk_command_buffer );
	vkCmdEndRenderPass( cmd->command_buffer );
}

static void
vk_cmd_barrier( const struct ft_command_buffer* icmd,
                uint32_t                        memory_barriers_count,
                const struct ft_memory_barrier* memory_barriers,
                uint32_t                        buffer_barriers_count,
                const struct ft_buffer_barrier* buffer_barriers,
                uint32_t                        image_barriers_count,
                const struct ft_image_barrier*  image_barriers )
{
	FT_UNUSED( memory_barriers_count );
	FT_UNUSED( memory_barriers );

	FT_FROM_HANDLE( cmd, icmd, vk_command_buffer );

	FT_ALLOC_STACK_ARRAY( VkBufferMemoryBarrier,
	                      buffer_memory_barriers,
	                      buffer_barriers_count );
	FT_ALLOC_STACK_ARRAY( VkImageMemoryBarrier,
	                      image_memory_barriers,
	                      image_barriers_count );

	// TODO: Queues
	VkAccessFlags src_access = ( VkAccessFlags ) 0;
	VkAccessFlags dst_access = ( VkAccessFlags ) 0;

	for ( uint32_t i = 0; i < buffer_barriers_count; ++i )
	{
		FT_ASSERT( buffer_barriers[ i ].buffer );

		FT_FROM_HANDLE( buffer, buffer_barriers[ i ].buffer, vk_buffer );

		VkAccessFlags src_access_mask =
		    determine_access_flags( buffer_barriers[ i ].old_state );
		VkAccessFlags dst_access_mask =
		    determine_access_flags( buffer_barriers[ i ].new_state );

		VkBufferMemoryBarrier* barrier = &buffer_memory_barriers[ i ];
		barrier->sType         = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier->pNext         = NULL;
		barrier->srcAccessMask = src_access_mask;
		barrier->dstAccessMask = dst_access_mask;
		barrier->srcQueueFamilyIndex =
		    buffer_barriers[ i ].src_queue
		        ? buffer_barriers[ i ].src_queue->family_index
		        : VK_QUEUE_FAMILY_IGNORED;
		barrier->dstQueueFamilyIndex =
		    buffer_barriers[ i ].dst_queue
		        ? buffer_barriers[ i ].dst_queue->family_index
		        : VK_QUEUE_FAMILY_IGNORED;
		barrier->buffer = buffer->buffer;
		barrier->offset = buffer_barriers[ i ].offset;
		barrier->size   = buffer_barriers[ i ].size;

		src_access |= src_access_mask;
		dst_access |= dst_access_mask;
	}

	for ( uint32_t i = 0; i < image_barriers_count; ++i )
	{
		FT_ASSERT( image_barriers[ i ].image );

		FT_FROM_HANDLE( image, image_barriers[ i ].image, vk_image );

		VkAccessFlags src_access_mask =
		    determine_access_flags( image_barriers[ i ].old_state );
		VkAccessFlags dst_access_mask =
		    determine_access_flags( image_barriers[ i ].new_state );

		VkImageMemoryBarrier* barrier = &image_memory_barriers[ i ];

		barrier->sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier->pNext         = NULL;
		barrier->srcAccessMask = src_access_mask;
		barrier->dstAccessMask = dst_access_mask;
		barrier->oldLayout =
		    determine_image_layout( image_barriers[ i ].old_state );
		barrier->newLayout =
		    determine_image_layout( image_barriers[ i ].new_state );
		barrier->srcQueueFamilyIndex =
		    image_barriers[ i ].src_queue
		        ? image_barriers[ i ].src_queue->family_index
		        : VK_QUEUE_FAMILY_IGNORED;
		barrier->dstQueueFamilyIndex =
		    image_barriers[ i ].dst_queue
		        ? image_barriers[ i ].dst_queue->family_index
		        : VK_QUEUE_FAMILY_IGNORED;
		barrier->image            = image->image;
		barrier->subresourceRange = get_image_subresource_range( image );

		src_access |= src_access_mask;
		dst_access |= dst_access_mask;
	}

	VkPipelineStageFlags src_stage =
	    determine_pipeline_stage_flags( src_access,
	                                    cmd->interface.queue->type );
	VkPipelineStageFlags dst_stage =
	    determine_pipeline_stage_flags( dst_access,
	                                    cmd->interface.queue->type );

	vkCmdPipelineBarrier( cmd->command_buffer,
	                      src_stage,
	                      dst_stage,
	                      0,
	                      0,
	                      NULL,
	                      buffer_barriers_count,
	                      buffer_memory_barriers,
	                      image_barriers_count,
	                      image_memory_barriers );
}

static void
vk_cmd_set_scissor( const struct ft_command_buffer* icmd,
                    int32_t                         x,
                    int32_t                         y,
                    uint32_t                        width,
                    uint32_t                        height )
{
	FT_FROM_HANDLE( cmd, icmd, vk_command_buffer );

	VkRect2D scissor = {
	    .offset.x      = x,
	    .offset.y      = y,
	    .extent.width  = width,
	    .extent.height = height,
	};
	vkCmdSetScissor( cmd->command_buffer, 0, 1, &scissor );
}

static void
vk_cmd_set_viewport( const struct ft_command_buffer* icmd,
                     float                           x,
                     float                           y,
                     float                           width,
                     float                           height,
                     float                           min_depth,
                     float                           max_depth )
{
	FT_FROM_HANDLE( cmd, icmd, vk_command_buffer );

	VkViewport viewport = {
	    .x        = x,
	    .y        = y + height,
	    .width    = width,
	    .height   = -height,
	    .minDepth = min_depth,
	    .maxDepth = max_depth,
	};

	vkCmdSetViewport( cmd->command_buffer, 0, 1, &viewport );
}

static void
vk_cmd_bind_pipeline( const struct ft_command_buffer* icmd,
                      const struct ft_pipeline*       ipipeline )
{
	FT_FROM_HANDLE( cmd, icmd, vk_command_buffer );
	FT_FROM_HANDLE( pipeline, ipipeline, vk_pipeline );

	vkCmdBindPipeline( cmd->command_buffer,
	                   to_vk_pipeline_bind_point( pipeline->interface.type ),
	                   pipeline->pipeline );
}

static void
vk_cmd_draw( const struct ft_command_buffer* icmd,
             uint32_t                        vertex_count,
             uint32_t                        instance_count,
             uint32_t                        first_vertex,
             uint32_t                        first_instance )
{
	FT_FROM_HANDLE( cmd, icmd, vk_command_buffer );

	vkCmdDraw( cmd->command_buffer,
	           vertex_count,
	           instance_count,
	           first_vertex,
	           first_instance );
}

static void
vk_cmd_draw_indexed( const struct ft_command_buffer* icmd,
                     uint32_t                        index_count,
                     uint32_t                        instance_count,
                     uint32_t                        first_index,
                     int32_t                         vertex_offset,
                     uint32_t                        first_instance )
{
	FT_FROM_HANDLE( cmd, icmd, vk_command_buffer );

	vkCmdDrawIndexed( cmd->command_buffer,
	                  index_count,
	                  instance_count,
	                  first_index,
	                  vertex_offset,
	                  first_instance );
}

static void
vk_cmd_bind_vertex_buffer( const struct ft_command_buffer* icmd,
                           const struct ft_buffer*         ibuffer,
                           const uint64_t                  offset )
{
	FT_FROM_HANDLE( cmd, icmd, vk_command_buffer );
	FT_FROM_HANDLE( buffer, ibuffer, vk_buffer );

	vkCmdBindVertexBuffers( cmd->command_buffer,
	                        0,
	                        1,
	                        &buffer->buffer,
	                        &offset );
}

static void
vk_cmd_bind_index_buffer( const struct ft_command_buffer* icmd,
                          const struct ft_buffer*         ibuffer,
                          uint64_t                        offset,
                          enum ft_index_type              index_type )
{
	FT_FROM_HANDLE( cmd, icmd, vk_command_buffer );
	FT_FROM_HANDLE( buffer, ibuffer, vk_buffer );

	vkCmdBindIndexBuffer( cmd->command_buffer,
	                      buffer->buffer,
	                      offset,
	                      to_vk_index_type( index_type ) );
}

static void
vk_cmd_copy_buffer( const struct ft_command_buffer* icmd,
                    const struct ft_buffer*         isrc,
                    uint64_t                        src_offset,
                    struct ft_buffer*               idst,
                    uint64_t                        dst_offset,
                    uint64_t                        size )
{
	FT_FROM_HANDLE( cmd, icmd, vk_command_buffer );
	FT_FROM_HANDLE( src, isrc, vk_buffer );
	FT_FROM_HANDLE( dst, idst, vk_buffer );

	VkBufferCopy buffer_copy = {
	    .srcOffset = src_offset,
	    .dstOffset = dst_offset,
	    .size      = size,
	};

	vkCmdCopyBuffer( cmd->command_buffer,
	                 src->buffer,
	                 dst->buffer,
	                 1,
	                 &buffer_copy );
}

static void
vk_cmd_copy_buffer_to_image( const struct ft_command_buffer*    icmd,
                             const struct ft_buffer*            isrc,
                             struct ft_image*                   idst,
                             const struct ft_buffer_image_copy* icopy )
{
	FT_FROM_HANDLE( cmd, icmd, vk_command_buffer );
	FT_FROM_HANDLE( src, isrc, vk_buffer );
	FT_FROM_HANDLE( dst, idst, vk_image );

	VkBufferImageCopy copy = {
	    .bufferOffset = icopy->buffer_offset,
	    .imageSubresource =
	        {
	            .aspectMask     = get_aspect_mask( idst->format ),
	            .mipLevel       = icopy->mip_level,
	            .baseArrayLayer = 0,
	            .layerCount     = idst->layer_count,
	        },
	    .imageOffset =
	        {
	            .x = 0,
	            .y = 0,
	            .z = 0,
	        },
	    .imageExtent =
	        {
	            .width  = icopy->width,
	            .height = icopy->height,
	            .depth  = 1,
	        },
	};

	vkCmdCopyBufferToImage( cmd->command_buffer,
	                        src->buffer,
	                        dst->image,
	                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	                        1,
	                        &copy );
}

static void
vk_cmd_dispatch( const struct ft_command_buffer* icmd,
                 uint32_t                        group_count_x,
                 uint32_t                        group_count_y,
                 uint32_t                        group_count_z )
{
	FT_FROM_HANDLE( cmd, icmd, vk_command_buffer );

	vkCmdDispatch( cmd->command_buffer,
	               group_count_x,
	               group_count_y,
	               group_count_z );
}

static void
vk_cmd_push_constants( const struct ft_command_buffer* icmd,
                       const struct ft_pipeline*       ipipeline,
                       uint32_t                        offset,
                       uint32_t                        size,
                       const void*                     data )
{
	FT_FROM_HANDLE( cmd, icmd, vk_command_buffer );
	FT_FROM_HANDLE( pipeline, ipipeline, vk_pipeline );

	vkCmdPushConstants( cmd->command_buffer,
	                    pipeline->pipeline_layout,
	                    VK_SHADER_STAGE_VERTEX_BIT |
	                        VK_SHADER_STAGE_COMPUTE_BIT |
	                        VK_SHADER_STAGE_FRAGMENT_BIT,
	                    offset,
	                    size,
	                    data );
}

static void
vk_cmd_clear_color_image( const struct ft_command_buffer* icmd,
                          struct ft_image*                iimage,
                          float                           color[ 4 ] )
{
	FT_FROM_HANDLE( cmd, icmd, vk_command_buffer );
	FT_FROM_HANDLE( image, iimage, vk_image );

	VkClearColorValue clear_color = {
	    .float32[ 0 ] = color[ 0 ],
	    .float32[ 1 ] = color[ 1 ],
	    .float32[ 2 ] = color[ 2 ],
	    .float32[ 3 ] = color[ 3 ],
	};
	VkImageSubresourceRange range = get_image_subresource_range( image );

	vkCmdClearColorImage( cmd->command_buffer,
	                      image->image,
	                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	                      &clear_color,
	                      1,
	                      &range );
}

static void
vk_cmd_draw_indexed_indirect( const struct ft_command_buffer* icmd,
                              const struct ft_buffer*         ibuffer,
                              uint64_t                        offset,
                              uint32_t                        draw_count,
                              uint32_t                        stride )
{
	FT_FROM_HANDLE( cmd, icmd, vk_command_buffer );
	FT_FROM_HANDLE( buffer, ibuffer, vk_buffer );

	vkCmdDrawIndexedIndirect( cmd->command_buffer,
	                          buffer->buffer,
	                          offset,
	                          draw_count,
	                          stride );
}

static void
vk_cmd_bind_descriptor_set( const struct ft_command_buffer* icmd,
                            uint32_t                        first_set,
                            const struct ft_descriptor_set* iset,
                            const struct ft_pipeline*       ipipeline )
{
	FT_FROM_HANDLE( cmd, icmd, vk_command_buffer );
	FT_FROM_HANDLE( pipeline, ipipeline, vk_pipeline );
	FT_FROM_HANDLE( set, iset, vk_descriptor_set );

	vkCmdBindDescriptorSets(
	    cmd->command_buffer,
	    to_vk_pipeline_bind_point( pipeline->interface.type ),
	    pipeline->pipeline_layout,
	    first_set,
	    1,
	    &set->descriptor_set,
	    0,
	    NULL );
}

void
vk_create_renderer_backend( const struct ft_renderer_backend_info* info,
                            struct ft_renderer_backend**           p )
{
	ft_destroy_renderer_backend_impl      = vk_destroy_renderer_backend;
	ft_create_device_impl                 = vk_create_device;
	ft_destroy_device_impl                = vk_destroy_device;
	ft_create_queue_impl                  = vk_create_queue;
	ft_destroy_queue_impl                 = vk_destroy_queue;
	ft_queue_wait_idle_impl               = vk_queue_wait_idle;
	ft_queue_submit_impl                  = vk_queue_submit;
	ft_immediate_submit_impl              = vk_immediate_submit;
	ft_queue_present_impl                 = vk_queue_present;
	ft_create_semaphore_impl              = vk_create_semaphore;
	ft_destroy_semaphore_impl             = vk_destroy_semaphore;
	ft_create_fence_impl                  = vk_create_fence;
	ft_destroy_fence_impl                 = vk_destroy_fence;
	ft_wait_for_fences_impl               = vk_wait_for_fences;
	ft_reset_fences_impl                  = vk_reset_fences;
	ft_create_swapchain_impl              = vk_create_swapchain;
	ft_resize_swapchain_impl              = vk_resize_swapchain;
	ft_destroy_swapchain_impl             = vk_destroy_swapchain;
	ft_create_command_pool_impl           = vk_create_command_pool;
	ft_destroy_command_pool_impl          = vk_destroy_command_pool;
	ft_create_command_buffers_impl        = vk_create_command_buffers;
	ft_free_command_buffers_impl          = vk_free_command_buffers;
	ft_destroy_command_buffers_impl       = vk_destroy_command_buffers;
	ft_begin_command_buffer_impl          = vk_begin_command_buffer;
	ft_end_command_buffer_impl            = vk_end_command_buffer;
	ft_acquire_next_image_impl            = vk_acquire_next_image;
	ft_create_shader_impl                 = vk_create_shader;
	ft_destroy_shader_impl                = vk_destroy_shader;
	ft_create_descriptor_set_layout_impl  = vk_create_descriptor_set_layout;
	ft_destroy_descriptor_set_layout_impl = vk_destroy_descriptor_set_layout;
	ft_create_pipeline_impl               = vk_create_pipeline;
	ft_destroy_pipeline_impl              = vk_destroy_pipeline;
	ft_create_buffer_impl                 = vk_create_buffer;
	ft_destroy_buffer_impl                = vk_destroy_buffer;
	ft_map_memory_impl                    = vk_map_memory;
	ft_unmap_memory_impl                  = vk_unmap_memory;
	ft_create_sampler_impl                = vk_create_sampler;
	ft_destroy_sampler_impl               = vk_destroy_sampler;
	ft_create_image_impl                  = vk_create_image;
	ft_destroy_image_impl                 = vk_destroy_image;
	ft_create_descriptor_set_impl         = vk_create_descriptor_set;
	ft_destroy_descriptor_set_impl        = vk_destroy_descriptor_set;
	ft_update_descriptor_set_impl         = vk_update_descriptor_set;
	ft_cmd_begin_render_pass_impl         = vk_cmd_begin_render_pass;
	ft_cmd_end_render_pass_impl           = vk_cmd_end_render_pass;
	ft_cmd_barrier_impl                   = vk_cmd_barrier;
	ft_cmd_set_scissor_impl               = vk_cmd_set_scissor;
	ft_cmd_set_viewport_impl              = vk_cmd_set_viewport;
	ft_cmd_bind_pipeline_impl             = vk_cmd_bind_pipeline;
	ft_cmd_draw_impl                      = vk_cmd_draw;
	ft_cmd_draw_indexed_impl              = vk_cmd_draw_indexed;
	ft_cmd_bind_vertex_buffer_impl        = vk_cmd_bind_vertex_buffer;
	ft_cmd_bind_index_buffer_impl         = vk_cmd_bind_index_buffer;
	ft_cmd_copy_buffer_impl               = vk_cmd_copy_buffer;
	ft_cmd_copy_buffer_to_image_impl      = vk_cmd_copy_buffer_to_image;
	ft_cmd_bind_descriptor_set_impl       = vk_cmd_bind_descriptor_set;
	ft_cmd_dispatch_impl                  = vk_cmd_dispatch;
	ft_cmd_push_constants_impl            = vk_cmd_push_constants;
	ft_cmd_draw_indexed_indirect_impl     = vk_cmd_draw_indexed_indirect;

	FT_INIT_INTERNAL( backend, *p, vk_renderer_backend );

	// TODO: provide posibility to set allocator from user code
	backend->vulkan_allocator = NULL;
	// TODO: same
	backend->api_version = VK_API_VERSION_1_2;

	volkInitialize();

	VkApplicationInfo app_info = {
	    .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
	    .pNext              = NULL,
	    .pApplicationName   = "Fluent",
	    .applicationVersion = VK_MAKE_VERSION( 0, 0, 1 ),
	    .pEngineName        = "Fluent-Engine",
	    .engineVersion      = VK_MAKE_VERSION( 0, 0, 1 ),
	    .apiVersion         = backend->api_version,
	};

	uint32_t instance_create_flags = 0;
	uint32_t extension_count       = 0;
	uint32_t layer_count           = 0;

	get_instance_extensions( info,
	                         &instance_create_flags,
	                         &extension_count,
	                         NULL );
	FT_ALLOC_STACK_ARRAY( const char*, extensions, extension_count );
	get_instance_extensions( info,
	                         &instance_create_flags,
	                         &extension_count,
	                         extensions );

	get_instance_layers( &layer_count, NULL );
	const char** layers = NULL;
	if ( layer_count != 0 )
	{
		layers = malloc( sizeof( const char* ) * layer_count );
		get_instance_layers( &layer_count, layers );
	}

	VkInstanceCreateInfo instance_create_info = {
	    .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
	    .pNext                   = NULL,
	    .pApplicationInfo        = &app_info,
	    .enabledLayerCount       = layer_count,
	    .ppEnabledLayerNames     = layers,
	    .enabledExtensionCount   = extension_count,
	    .ppEnabledExtensionNames = extensions,
	    .flags                   = instance_create_flags,
	};

	VK_ASSERT( vkCreateInstance( &instance_create_info,
	                             backend->vulkan_allocator,
	                             &backend->instance ) );
	if ( layers != NULL )
	{
		free( layers );
	}

	volkLoadInstance( backend->instance );

#ifdef FLUENT_DEBUG
	create_debug_messenger( backend );
#endif

	// pick physical device
	backend->physical_device = VK_NULL_HANDLE;
	uint32_t device_count    = 0;
	vkEnumeratePhysicalDevices( backend->instance, &device_count, NULL );
	FT_ASSERT( device_count != 0 );
	FT_ALLOC_STACK_ARRAY( VkPhysicalDevice, physical_devices, device_count );
	vkEnumeratePhysicalDevices( backend->instance,
	                            &device_count,
	                            physical_devices );
	backend->physical_device = physical_devices[ 0 ];

	// select best physical device
	for ( uint32_t i = 0; i < device_count; ++i )
	{
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures   deviceFeatures;
		vkGetPhysicalDeviceProperties( physical_devices[ i ],
		                               &deviceProperties );
		vkGetPhysicalDeviceFeatures( physical_devices[ i ], &deviceFeatures );

		if ( deviceProperties.deviceType ==
		     VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU )
		{
			backend->physical_device = physical_devices[ i ];
			break;
		}
	}
}

#endif
