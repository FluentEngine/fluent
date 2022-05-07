#ifdef VULKAN_BACKEND

#include <algorithm>
#include <SDL_vulkan.h>
#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_sdl.h>
#include <tinyimageformat_apis.h>
#include <vk_enum_string_helper.h>
#include "core/window.hpp"
#include "core/application.hpp"
#include "fs/fs.hpp"
#include "vulkan_backend.hpp"

#ifdef FLUENT_DEBUG
#define VK_ASSERT( x )                                                         \
	do {                                                                       \
	    VkResult err = x;                                                      \
	    if ( err )                                                             \
        {                                                                      \
	        FT_ERROR( "Detected Vulkan error: {}", err );                      \
	        abort();                                                           \
	    }                                                                      \
	} while ( 0 )
#else
#define VK_ASSERT( x ) x
#endif

namespace fluent
{
static inline VkFormat
to_vk_format( Format format )
{
	return static_cast<VkFormat>(
	    TinyImageFormat_ToVkFormat( static_cast<TinyImageFormat>( format ) ) );
}

static inline Format
from_vk_format( VkFormat format )
{
	return static_cast<Format>( TinyImageFormat_FromVkFormat(
	    static_cast<TinyImageFormat_VkFormat>( format ) ) );
}

static inline VkSampleCountFlagBits
to_vk_sample_count( SampleCount sample_count )
{
	switch ( sample_count )
	{
	case SampleCount::e1: return VK_SAMPLE_COUNT_1_BIT;
	case SampleCount::e2: return VK_SAMPLE_COUNT_2_BIT;
	case SampleCount::e4: return VK_SAMPLE_COUNT_4_BIT;
	case SampleCount::e8: return VK_SAMPLE_COUNT_8_BIT;
	case SampleCount::e16: return VK_SAMPLE_COUNT_16_BIT;
	case SampleCount::e32: return VK_SAMPLE_COUNT_32_BIT;
	default: FT_ASSERT( false ); return VkSampleCountFlagBits( -1 );
	}
}

static inline VkAttachmentLoadOp
to_vk_load_op( AttachmentLoadOp load_op )
{
	switch ( load_op )
	{
	case AttachmentLoadOp::eClear: return VK_ATTACHMENT_LOAD_OP_CLEAR;
	case AttachmentLoadOp::eDontCare: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	case AttachmentLoadOp::eLoad: return VK_ATTACHMENT_LOAD_OP_LOAD;
	default: FT_ASSERT( false ); return VkAttachmentLoadOp( -1 );
	}
}

static inline VkQueueFlagBits
to_vk_queue_type( QueueType type )
{
	switch ( type )
	{
	case QueueType::eGraphics: return VK_QUEUE_GRAPHICS_BIT;
	case QueueType::eCompute: return VK_QUEUE_COMPUTE_BIT;
	case QueueType::eTransfer: return VK_QUEUE_TRANSFER_BIT;
	default: FT_ASSERT( false ); return VkQueueFlagBits( -1 );
	}
}

static inline VkVertexInputRate
to_vk_vertex_input_rate( VertexInputRate input_rate )
{
	switch ( input_rate )
	{
	case VertexInputRate::eVertex: return VK_VERTEX_INPUT_RATE_VERTEX;
	case VertexInputRate::eInstance: return VK_VERTEX_INPUT_RATE_INSTANCE;
	default: FT_ASSERT( false ); return VkVertexInputRate( -1 );
	}
}

static inline VkCullModeFlagBits
to_vk_cull_mode( CullMode cull_mode )
{
	switch ( cull_mode )
	{
	case CullMode::eBack: return VK_CULL_MODE_BACK_BIT;
	case CullMode::eFront: return VK_CULL_MODE_FRONT_BIT;
	case CullMode::eNone: return VK_CULL_MODE_NONE;
	default: FT_ASSERT( false ); return VkCullModeFlagBits( -1 );
	}
}

static inline VkFrontFace
to_vk_front_face( FrontFace front_face )
{
	switch ( front_face )
	{
	case FrontFace::eClockwise: return VK_FRONT_FACE_CLOCKWISE;
	case FrontFace::eCounterClockwise: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
	default: FT_ASSERT( false ); return VkFrontFace( -1 );
	}
}

static inline VkCompareOp
to_vk_compare_op( CompareOp op )
{
	switch ( op )
	{
	case CompareOp::eNever: return VK_COMPARE_OP_NEVER;
	case CompareOp::eLess: return VK_COMPARE_OP_LESS;
	case CompareOp::eEqual: return VK_COMPARE_OP_EQUAL;
	case CompareOp::eLessOrEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
	case CompareOp::eGreater: return VK_COMPARE_OP_GREATER;
	case CompareOp::eNotEqual: return VK_COMPARE_OP_NOT_EQUAL;
	case CompareOp::eGreaterOrEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
	case CompareOp::eAlways: return VK_COMPARE_OP_ALWAYS;
	default: FT_ASSERT( false ); return VkCompareOp( -1 );
	}
}

static inline VkShaderStageFlagBits
to_vk_shader_stage( ShaderStage shader_stage )
{
	switch ( shader_stage )
	{
	case ShaderStage::eVertex: return VK_SHADER_STAGE_VERTEX_BIT;
	case ShaderStage::eTessellationControl:
		return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	case ShaderStage::eTessellationEvaluation:
		return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	case ShaderStage::eGeometry: return VK_SHADER_STAGE_GEOMETRY_BIT;
	case ShaderStage::eFragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
	case ShaderStage::eCompute: return VK_SHADER_STAGE_COMPUTE_BIT;
	case ShaderStage::eAllGraphics: return VK_SHADER_STAGE_ALL_GRAPHICS;
	case ShaderStage::eAll: return VK_SHADER_STAGE_ALL;
	default: FT_ASSERT( false ); return VkShaderStageFlagBits( -1 );
	}
}

static inline VkPipelineBindPoint
to_vk_pipeline_bind_point( PipelineType type )
{
	switch ( type )
	{
	case PipelineType::eCompute: return VK_PIPELINE_BIND_POINT_COMPUTE;
	case PipelineType::eGraphics: return VK_PIPELINE_BIND_POINT_GRAPHICS;
	default: FT_ASSERT( false ); return VkPipelineBindPoint( -1 );
	}
}

static inline VkDescriptorType
to_vk_descriptor_type( DescriptorType descriptor_type )
{
	switch ( descriptor_type )
	{
	case DescriptorType::eSampler: return VK_DESCRIPTOR_TYPE_SAMPLER;
	case DescriptorType::eSampledImage: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	case DescriptorType::eStorageImage: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	case DescriptorType::eUniformTexelBuffer:
		return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
	case DescriptorType::eStorageTexelBuffer:
		return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
	case DescriptorType::eUniformBuffer:
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	case DescriptorType::eStorageBuffer:
		return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	case DescriptorType::eUniformBufferDynamic:
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	case DescriptorType::eStorageBufferDynamic:
		return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
	case DescriptorType::eInputAttachment:
		return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	default: FT_ASSERT( false ); return VkDescriptorType( -1 );
	}
}

static inline VkFilter
to_vk_filter( Filter filter )
{
	switch ( filter )
	{
	case Filter::eLinear: return VK_FILTER_LINEAR;
	case Filter::eNearest: return VK_FILTER_NEAREST;
	default: FT_ASSERT( false ); return VkFilter( -1 );
	}
}

static inline VkSamplerMipmapMode
to_vk_sampler_mipmap_mode( SamplerMipmapMode mode )
{
	switch ( mode )
	{
	case SamplerMipmapMode::eNearest:
		return VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_NEAREST;
	case SamplerMipmapMode::eLinear:
		return VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_LINEAR;
	default: FT_ASSERT( false ); return VkSamplerMipmapMode( -1 );
	}
}

static inline VkSamplerAddressMode
to_vk_sampler_address_mode( SamplerAddressMode mode )
{
	switch ( mode )
	{
	case SamplerAddressMode::eRepeat:
		return VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case SamplerAddressMode::eMirroredRepeat:
		return VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	case SamplerAddressMode::eClampToEdge:
		return VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case SamplerAddressMode::eClampToBorder:
		return VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	default: FT_ASSERT( false ); return VkSamplerAddressMode( -1 );
	}
}

static inline VkPolygonMode
to_vk_polygon_mode( PolygonMode mode )
{
	switch ( mode )
	{
	case PolygonMode::eFill: return VK_POLYGON_MODE_FILL;
	case PolygonMode::eLine: return VK_POLYGON_MODE_LINE;
	default: FT_ASSERT( false ); return VkPolygonMode( -1 );
	}
};

static inline VkPrimitiveTopology
to_vk_primitive_topology( PrimitiveTopology topology )
{
	switch ( topology )
	{
	case PrimitiveTopology::eLineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	case PrimitiveTopology::eLineStrip: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
	case PrimitiveTopology::ePointList: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	case PrimitiveTopology::eTriangleFan:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
	case PrimitiveTopology::eTriangleStrip:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	case PrimitiveTopology::eTriangleList:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	default: FT_ASSERT( false ); return VkPrimitiveTopology( -1 );
	}
}

static inline VkAccessFlags
determine_access_flags( ResourceState resource_state )
{
	VkAccessFlags access_flags = 0;

	if ( b32( resource_state & ResourceState::eGeneral ) )
		access_flags |=
		    ( VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT );

	if ( b32( resource_state & ResourceState::eColorAttachment ) )
		access_flags |= ( VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
		                  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT );

	if ( b32( resource_state & ResourceState::eDepthStencilWrite ) )
		access_flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	if ( b32( resource_state & ResourceState::eDepthStencilReadOnly ) )
		access_flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

	if ( b32( resource_state & ResourceState::eShaderReadOnly ) )
		access_flags |= VK_ACCESS_SHADER_READ_BIT;

	if ( b32( resource_state & ResourceState::eTransferSrc ) )
		access_flags |= VK_ACCESS_TRANSFER_READ_BIT;

	if ( b32( resource_state & ResourceState::eTransferDst ) )
		access_flags |= VK_ACCESS_TRANSFER_WRITE_BIT;

	if ( b32( resource_state & ResourceState::ePresent ) )
		access_flags |= VK_ACCESS_MEMORY_READ_BIT;

	return access_flags;
}

static inline VkPipelineStageFlags
determine_pipeline_stage_flags( VkAccessFlags access_flags,
                                QueueType     queue_type )
{
	// TODO: FIX THIS
	VkPipelineStageFlags flags = 0;

	switch ( queue_type )
	{
	case QueueType::eGraphics:
	{
		if ( access_flags & ( VK_ACCESS_INDEX_READ_BIT |
		                      VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT ) )
		{
			flags |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
		}

		if ( access_flags &
		     ( VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT |
		       VK_ACCESS_SHADER_WRITE_BIT ) )
		{
			flags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
			flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}

		if ( access_flags & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT )
		{
			flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}

		if ( access_flags & ( VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
		                      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT ) )
		{
			flags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}

		if ( access_flags & ( VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
		                      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT ) )
		{
			flags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
			         VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		}
		break;
	}
	case QueueType::eCompute:
	{
	}
	default:
	{
	}
	}

	if ( access_flags & ( VK_ACCESS_INDEX_READ_BIT |
	                      VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT ) ||
	     access_flags & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT ||
	     access_flags & ( VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
	                      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT ) ||
	     access_flags & ( VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
	                      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT ) )
		return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

	if ( access_flags &
	     ( VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT |
	       VK_ACCESS_SHADER_WRITE_BIT ) )
		flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

	if ( access_flags & VK_ACCESS_INDIRECT_COMMAND_READ_BIT )
		flags |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;

	if ( access_flags &
	     ( VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT ) )
		flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;

	if ( access_flags & ( VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT ) )
		flags |= VK_PIPELINE_STAGE_HOST_BIT;

	if ( flags == 0 )
		flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	return flags;
}

static inline VkImageLayout
determine_image_layout( ResourceState resource_state )
{
	if ( b32( resource_state & ResourceState::eGeneral ) )
		return VK_IMAGE_LAYOUT_GENERAL;

	if ( b32( resource_state & ResourceState::eColorAttachment ) )
		return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	if ( b32( resource_state & ResourceState::eDepthStencilWrite ) )
		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	if ( b32( resource_state & ResourceState::eDepthStencilReadOnly ) )
		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

	if ( b32( resource_state & ResourceState::eShaderReadOnly ) )
		return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	if ( b32( resource_state & ResourceState::ePresent ) )
		return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	if ( b32( resource_state & ResourceState::eTransferSrc ) )
		return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

	if ( b32( resource_state & ResourceState::eTransferDst ) )
		return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

	return VK_IMAGE_LAYOUT_UNDEFINED;
}

static inline VmaMemoryUsage
determine_vma_memory_usage( MemoryUsage usage )
{
	switch ( usage )
	{
	case MemoryUsage::eCpuOnly: return VMA_MEMORY_USAGE_CPU_ONLY;
	case MemoryUsage::eGpuOnly: return VMA_MEMORY_USAGE_GPU_ONLY;
	case MemoryUsage::eCpuCopy: return VMA_MEMORY_USAGE_CPU_COPY;
	case MemoryUsage::eCpuToGpu: return VMA_MEMORY_USAGE_CPU_TO_GPU;
	case MemoryUsage::eGpuToCpu: return VMA_MEMORY_USAGE_GPU_TO_CPU;
	default: break;
	}
	FT_ASSERT( false );
	return VmaMemoryUsage( -1 );
}

static inline VkBufferUsageFlags
determine_vk_buffer_usage( DescriptorType descriptor_type,
                           MemoryUsage    memory_usage )
{
	// TODO: determine buffer usage flags
	VkBufferUsageFlags buffer_usage = VkBufferUsageFlags( 0 );

	if ( memory_usage == MemoryUsage::eCpuToGpu ||
	     memory_usage == MemoryUsage::eCpuOnly )
	{
		buffer_usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	}

	if ( b32( descriptor_type & DescriptorType::eVertexBuffer ) )
	{
		buffer_usage |= ( VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
		                  VK_BUFFER_USAGE_TRANSFER_DST_BIT );
	}

	if ( b32( descriptor_type & DescriptorType::eIndexBuffer ) )
	{
		buffer_usage |= ( VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
		                  VK_BUFFER_USAGE_TRANSFER_DST_BIT );
	}

	if ( b32( descriptor_type & DescriptorType::eUniformBuffer ) )
	{
		buffer_usage |= ( VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
		                  VK_BUFFER_USAGE_TRANSFER_DST_BIT );
	}

	if ( b32( descriptor_type & DescriptorType::eIndirectBuffer ) )
	{
		buffer_usage |= ( VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
		                  VK_BUFFER_USAGE_TRANSFER_DST_BIT );
	}

	if ( b32( descriptor_type & DescriptorType::eStorageBuffer ) )
	{
		buffer_usage |= ( VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		                  VK_BUFFER_USAGE_TRANSFER_DST_BIT );
	}

	return buffer_usage;
}

static inline VkImageUsageFlags
determine_vk_image_usage( DescriptorType descriptor_type )
{
	VkImageUsageFlags image_usage = VkImageUsageFlags( 0 );

	if ( b32( descriptor_type & DescriptorType::eSampledImage ) )
	{
		image_usage |= VK_IMAGE_USAGE_SAMPLED_BIT |
		               VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
		               VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	if ( b32( descriptor_type & DescriptorType::eStorageImage ) )
	{
		image_usage |=
		    VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	if ( b32( descriptor_type & DescriptorType::eInputAttachment ) )
	{
		image_usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
	}

	if ( b32( descriptor_type & DescriptorType::eColorAttachment ) )
	{
		image_usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}

	if ( b32( descriptor_type & DescriptorType::eDepthStencilAttachment ) )
	{
		image_usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}

	if ( b32( descriptor_type & DescriptorType::eTransientAttachment ) )
	{
		image_usage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
	}

	return image_usage;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
vulkan_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* )
{
	if ( messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT )
	{
		FT_TRACE( "[ vulkan validation layer ] {}", pCallbackData->pMessage );
	}
	else if ( messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT )
	{
		FT_INFO( "[ vulkan validation layer ] {}", pCallbackData->pMessage );
	}
	else if ( messageSeverity ==
	          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT )
	{
		FT_WARN( "[ vulkan validation layer ] {}", pCallbackData->pMessage );
	}
	else if ( messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT )
	{
		FT_ERROR( "[ vulkan validation layer ] {}", pCallbackData->pMessage );
	}

	return VK_FALSE;
}

static inline void
create_debug_messenger( VulkanRendererBackend* backend )
{
	VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info {};
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
	debug_messenger_create_info.pUserData       = nullptr;

	vkCreateDebugUtilsMessengerEXT( backend->instance,
	                                &debug_messenger_create_info,
	                                backend->vulkan_allocator,
	                                &backend->debug_messenger );
}

static inline std::vector<const char*>
get_instance_extensions( u32& instance_create_flags )
{
	instance_create_flags = 0;

	u32 extension_count = 0;
	b32 result          = SDL_Vulkan_GetInstanceExtensions(
	    ( SDL_Window* ) get_app_window()->handle,
	    &extension_count,
	    nullptr );
	FT_ASSERT( result );
	std::vector<const char*> instance_extensions( extension_count );
	SDL_Vulkan_GetInstanceExtensions( ( SDL_Window* ) get_app_window()->handle,
	                                  &extension_count,
	                                  instance_extensions.data() );

	vkEnumerateInstanceExtensionProperties( nullptr,
	                                        &extension_count,
	                                        nullptr );
	std::vector<VkExtensionProperties> extension_properties( extension_count );
	vkEnumerateInstanceExtensionProperties( nullptr,
	                                        &extension_count,
	                                        extension_properties.data() );

	for ( u32 i = 0; i < extension_count; ++i )
	{
		if ( std::strcmp( VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
		                  extension_properties[ i ].extensionName ) == 0 )
		{
			instance_extensions.emplace_back(
			    VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME );
			instance_create_flags |= VK_KHR_portability_enumeration;
		}

#ifdef FLUENT_DEBUG
		if ( std::strcmp( VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		                  extension_properties[ i ].extensionName ) == 0 )
		{
			instance_extensions.emplace_back(
			    VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
		}
#endif
	}

	return instance_extensions;
}

static inline std::vector<const char*>
get_instance_layers()
{
	std::vector<const char*> instance_layers;
#ifdef FLUENT_DEBUG
	instance_layers.emplace_back( "VK_LAYER_KHRONOS_validation" );
#endif
	return instance_layers;
}

static inline u32
find_queue_family_index( VkPhysicalDevice physical_device,
                         QueueType        queue_type )
{
	u32 index = std::numeric_limits<u32>::max();

	u32 queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties( physical_device,
	                                          &queue_family_count,
	                                          nullptr );
	std::vector<VkQueueFamilyProperties> queue_families( queue_family_count );
	vkGetPhysicalDeviceQueueFamilyProperties( physical_device,
	                                          &queue_family_count,
	                                          queue_families.data() );

	VkQueueFlagBits target_queue_type = to_vk_queue_type( queue_type );

	// TODO: Find dedicated queue first
	for ( u32 i = 0; i < queue_family_count; ++i )
	{
		if ( queue_families[ i ].queueFlags & target_queue_type )
		{
			index = i;
			break;
		}
	}

	// TODO: Rewrite more elegant way
	FT_ASSERT( index != std::numeric_limits<u32>::max() );

	return index;
}

static inline VkImageAspectFlags
get_aspect_mask( Format format )
{
	VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;

	if ( format_has_depth_aspect( format ) )
	{
		aspect_mask &= ~VK_IMAGE_ASPECT_COLOR_BIT;
		aspect_mask |= VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else if ( format_has_stencil_aspect( format ) )
	{
		aspect_mask &= ~VK_IMAGE_ASPECT_COLOR_BIT;
		aspect_mask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	return aspect_mask;
}

static inline VkImageSubresourceRange
get_image_subresource_range( const VulkanImage* image )
{
	VkImageSubresourceRange image_subresource_range {};
	image_subresource_range.aspectMask =
	    get_aspect_mask( image->interface.format );
	image_subresource_range.baseMipLevel   = 0;
	image_subresource_range.levelCount     = image->interface.mip_level_count;
	image_subresource_range.baseArrayLayer = 0;
	image_subresource_range.layerCount     = image->interface.layer_count;

	return image_subresource_range;
}

static inline VkImageSubresourceLayers
get_image_subresource_layers( const VulkanImage* image )
{
	auto subresourceRange = get_image_subresource_range( image );
	return VkImageSubresourceLayers { subresourceRange.aspectMask,
		                              subresourceRange.baseMipLevel,
		                              subresourceRange.baseArrayLayer,
		                              subresourceRange.layerCount };
}

void
vk_destroy_renderer_backend( RendererBackend* ibackend )
{
	FT_ASSERT( ibackend );

	FT_FROM_HANDLE( backend, ibackend, VulkanRendererBackend );

#ifdef FLUENT_DEBUG
	vkDestroyDebugUtilsMessengerEXT( backend->instance,
	                                 backend->debug_messenger,
	                                 backend->vulkan_allocator );
#endif
	vkDestroyInstance( backend->instance, backend->vulkan_allocator );
	std::free( backend );
}

void*
vk_map_memory( const Device* idevice, Buffer* ibuffer )
{
	FT_ASSERT( ibuffer != nullptr );
	FT_ASSERT( ibuffer->mapped_memory == nullptr );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( buffer, ibuffer, VulkanBuffer );

	vmaMapMemory( device->memory_allocator,
	              buffer->allocation,
	              &buffer->interface.mapped_memory );

	return buffer->interface.mapped_memory;
}

void
vk_unmap_memory( const Device* idevice, Buffer* ibuffer )
{
	FT_ASSERT( ibuffer );
	FT_ASSERT( ibuffer->mapped_memory );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( buffer, ibuffer, VulkanBuffer );

	vmaUnmapMemory( device->memory_allocator, buffer->allocation );
	buffer->interface.mapped_memory = nullptr;
}

void
vk_create_device( const RendererBackend* ibackend,
                  const DeviceInfo*      info,
                  Device**               p )
{
	FT_ASSERT( p );
	FT_ASSERT( info->frame_in_use_count > 0 );

	FT_FROM_HANDLE( backend, ibackend, VulkanRendererBackend );

	FT_INIT_INTERNAL( device, *p, VulkanDevice );

	device->vulkan_allocator = backend->vulkan_allocator;
	device->instance         = backend->instance;
	device->physical_device  = backend->physical_device;

	u32 queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties( device->physical_device,
	                                          &queue_family_count,
	                                          nullptr );
	std::vector<VkQueueFamilyProperties> queue_families( queue_family_count );
	vkGetPhysicalDeviceQueueFamilyProperties( device->physical_device,
	                                          &queue_family_count,
	                                          queue_families.data() );

	u32 queue_create_info_count = 0;
	VkDeviceQueueCreateInfo
	    queue_create_infos[ static_cast<int>( QueueType::eLast ) ];
	f32 queue_priority = 1.0f;

	// TODO: Select queues
	for ( u32 i = 0; i < queue_family_count; ++i )
	{
		queue_create_infos[ queue_create_info_count ] = {};
		queue_create_infos[ queue_create_info_count ].sType =
		    VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_infos[ queue_create_info_count ].pNext      = nullptr;
		queue_create_infos[ queue_create_info_count ].flags      = 0;
		queue_create_infos[ queue_create_info_count ].queueCount = 1;
		queue_create_infos[ queue_create_info_count ].pQueuePriorities =
		    &queue_priority;
		queue_create_infos[ queue_create_info_count ].queueFamilyIndex = i;
		queue_create_info_count++;

		if ( queue_create_info_count == static_cast<i32>( QueueType::eLast ) )
			break;
	}

	std::vector<const char*> device_extensions {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
	};

	u32                                device_extension_count = 0;
	std::vector<VkExtensionProperties> supported_device_extensions;
	vkEnumerateDeviceExtensionProperties( device->physical_device,
	                                      nullptr,
	                                      &device_extension_count,
	                                      nullptr );
	supported_device_extensions.resize( device_extension_count );
	vkEnumerateDeviceExtensionProperties( device->physical_device,
	                                      nullptr,
	                                      &device_extension_count,
	                                      supported_device_extensions.data() );
	for ( u32 i = 0; i < device_extension_count; ++i )
	{
		if ( !std::strcmp( supported_device_extensions[ i ].extensionName,
		                   "VK_KHR_portability_subset" ) )
		{
			device_extensions.emplace_back( "VK_KHR_portability_subset" );
		}
	}

	// TODO: check support
	VkPhysicalDeviceFeatures used_features {};
	used_features.fillModeNonSolid  = VK_TRUE;
	used_features.multiDrawIndirect = VK_TRUE;
	used_features.sampleRateShading = VK_TRUE;
	used_features.samplerAnisotropy = VK_TRUE;

	VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features {};
	descriptor_indexing_features.sType =
	    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
	descriptor_indexing_features.descriptorBindingPartiallyBound = true;

	VkPhysicalDeviceMultiviewFeatures multiview_features {};
	multiview_features.sType =
	    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES;
	multiview_features.multiview = true;
	multiview_features.pNext     = &descriptor_indexing_features;

	VkPhysicalDeviceShaderDrawParametersFeatures
	    shader_draw_parameters_features {};
	shader_draw_parameters_features.sType =
	    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETER_FEATURES;
	shader_draw_parameters_features.shaderDrawParameters = true;
	shader_draw_parameters_features.pNext                = &multiview_features;

	VkDeviceCreateInfo device_create_info {};
	device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.pNext = &shader_draw_parameters_features;
	device_create_info.flags = 0;
	device_create_info.queueCreateInfoCount = queue_create_info_count;
	device_create_info.pQueueCreateInfos    = queue_create_infos;
	device_create_info.enabledLayerCount    = 0;
	device_create_info.ppEnabledLayerNames  = nullptr;
	device_create_info.enabledExtensionCount =
	    static_cast<u32>( device_extensions.size() );
	device_create_info.ppEnabledExtensionNames = device_extensions.data();
	device_create_info.pEnabledFeatures        = &used_features;

	VK_ASSERT( vkCreateDevice( device->physical_device,
	                           &device_create_info,
	                           device->vulkan_allocator,
	                           &device->logical_device ) );

	volkLoadDevice( device->logical_device );

	VmaVulkanFunctions vulkanFunctions    = {};
	vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vulkanFunctions.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;
	vulkanFunctions.vkAllocateMemory      = vkAllocateMemory;
	vulkanFunctions.vkBindBufferMemory    = vkBindBufferMemory;
	vulkanFunctions.vkBindImageMemory     = vkBindImageMemory;
	vulkanFunctions.vkCreateBuffer        = vkCreateBuffer;
	vulkanFunctions.vkCreateImage         = vkCreateImage;
	vulkanFunctions.vkDestroyBuffer       = vkDestroyBuffer;
	vulkanFunctions.vkDestroyImage        = vkDestroyImage;
	vulkanFunctions.vkFreeMemory          = vkFreeMemory;
	vulkanFunctions.vkGetBufferMemoryRequirements =
	    vkGetBufferMemoryRequirements;
	vulkanFunctions.vkGetBufferMemoryRequirements2KHR =
	    vkGetBufferMemoryRequirements2KHR;
	vulkanFunctions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
	vulkanFunctions.vkGetImageMemoryRequirements2KHR =
	    vkGetImageMemoryRequirements2KHR;
	vulkanFunctions.vkGetPhysicalDeviceMemoryProperties =
	    vkGetPhysicalDeviceMemoryProperties;
	vulkanFunctions.vkGetPhysicalDeviceProperties =
	    vkGetPhysicalDeviceProperties;
	vulkanFunctions.vkMapMemory               = vkMapMemory;
	vulkanFunctions.vkUnmapMemory             = vkUnmapMemory;
	vulkanFunctions.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
	vulkanFunctions.vkInvalidateMappedMemoryRanges =
	    vkInvalidateMappedMemoryRanges;
	vulkanFunctions.vkCmdCopyBuffer = vkCmdCopyBuffer;
#if VMA_BIND_MEMORY2 || VMA_VULKAN_VERSION >= 1001000
	vulkanFunctions.vkBindBufferMemory2KHR = vkBindBufferMemory2KHR;
	vulkanFunctions.vkBindImageMemory2KHR  = vkBindImageMemory2KHR;
#endif
#if VMA_MEMORY_BUDGET || VMA_VULKAN_VERSION >= 1001000
	vulkanFunctions.vkGetPhysicalDeviceMemoryProperties2KHR =
	    vkGetPhysicalDeviceMemoryProperties2KHR;
#endif
#if VMA_VULKAN_VERSION >= 1003000
	vulkanFunctions.vkGetDeviceBufferMemoryRequirements =
	    vkGetDeviceBufferMemoryRequirementsKHR;
	vulkanFunctions.vkGetDeviceImageMemoryRequirements =
	    vkGetDeviceImageMemoryRequirements;
#endif

	VmaAllocatorCreateInfo vma_allocator_create_info {};
	vma_allocator_create_info.instance             = device->instance;
	vma_allocator_create_info.physicalDevice       = device->physical_device;
	vma_allocator_create_info.device               = device->logical_device;
	vma_allocator_create_info.flags                = 0;
	vma_allocator_create_info.pAllocationCallbacks = device->vulkan_allocator;
	vma_allocator_create_info.pVulkanFunctions     = &vulkanFunctions;
	vma_allocator_create_info.vulkanApiVersion     = backend->api_version;

	VK_ASSERT( vmaCreateAllocator( &vma_allocator_create_info,
	                               &device->memory_allocator ) );

	static constexpr u32 pool_size_count               = 11;
	VkDescriptorPoolSize pool_sizes[ pool_size_count ] = {
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

	VkDescriptorPoolCreateInfo descriptor_pool_create_info {};
	descriptor_pool_create_info.sType =
	    VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptor_pool_create_info.pNext = nullptr;
	descriptor_pool_create_info.flags =
	    VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	descriptor_pool_create_info.maxSets       = 2048 * pool_size_count;
	descriptor_pool_create_info.poolSizeCount = pool_size_count;
	descriptor_pool_create_info.pPoolSizes    = pool_sizes;

	VK_ASSERT( vkCreateDescriptorPool( device->logical_device,
	                                   &descriptor_pool_create_info,
	                                   device->vulkan_allocator,
	                                   &device->descriptor_pool ) );
}

void
vk_destroy_device( Device* idevice )
{
	FT_ASSERT( idevice );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	vkDestroyDescriptorPool( device->logical_device,
	                         device->descriptor_pool,
	                         device->vulkan_allocator );
	vmaDestroyAllocator( device->memory_allocator );
	vkDestroyDevice( device->logical_device, device->vulkan_allocator );

	free( device );
}

void
vk_create_queue( const Device* idevice, const QueueInfo* info, Queue** p )
{
	FT_ASSERT( p );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	FT_INIT_INTERNAL( queue, *p, VulkanQueue );

	u32 index =
	    find_queue_family_index( device->physical_device, info->queue_type );

	queue->interface.family_index = index;
	queue->interface.type         = info->queue_type;
	vkGetDeviceQueue( device->logical_device, index, 0, &queue->queue );
}

void
vk_destroy_queue( Queue* iqueue )
{
	FT_ASSERT( iqueue );
	FT_FROM_HANDLE( queue, iqueue, VulkanQueue );
	std::free( queue );
}

void
vk_queue_wait_idle( const Queue* iqueue )
{
	FT_FROM_HANDLE( queue, iqueue, VulkanQueue );
	vkQueueWaitIdle( queue->queue );
}

void
vk_queue_submit( const Queue* iqueue, const QueueSubmitInfo* info )
{
	FT_FROM_HANDLE( queue, iqueue, VulkanQueue );

	VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
	std::vector<VkSemaphore>     wait_semaphores( info->wait_semaphore_count );
	std::vector<VkCommandBuffer> command_buffers( info->command_buffer_count );
	std::vector<VkSemaphore> signal_semaphores( info->signal_semaphore_count );

	for ( u32 i = 0; i < info->wait_semaphore_count; ++i )
	{
		FT_FROM_HANDLE( semaphore,
		                info->wait_semaphores[ i ],
		                VulkanSemaphore );

		wait_semaphores[ i ] = semaphore->semaphore;
	}

	for ( u32 i = 0; i < info->command_buffer_count; ++i )
	{
		FT_FROM_HANDLE( cmd, info->command_buffers[ i ], VulkanCommandBuffer );
		command_buffers[ i ] = cmd->command_buffer;
	}

	for ( u32 i = 0; i < info->signal_semaphore_count; ++i )
	{
		FT_FROM_HANDLE( semaphore,
		                info->signal_semaphores[ i ],
		                VulkanSemaphore );
		signal_semaphores[ i ] = semaphore->semaphore;
	}

	VkSubmitInfo submit_info         = {};
	submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext                = nullptr;
	submit_info.waitSemaphoreCount   = info->wait_semaphore_count;
	submit_info.pWaitSemaphores      = wait_semaphores.data();
	submit_info.pWaitDstStageMask    = &wait_dst_stage_mask;
	submit_info.commandBufferCount   = info->command_buffer_count;
	submit_info.pCommandBuffers      = command_buffers.data();
	submit_info.signalSemaphoreCount = info->signal_semaphore_count;
	submit_info.pSignalSemaphores    = signal_semaphores.data();

	vkQueueSubmit(
	    queue->queue,
	    1,
	    &submit_info,
	    info->signal_fence
	        ? static_cast<VulkanFence*>( info->signal_fence->handle )->fence
	        : VK_NULL_HANDLE );
}

void
vk_immediate_submit( const Queue* iqueue, CommandBuffer* cmd )
{
	QueueSubmitInfo queue_submit_info {};
	queue_submit_info.command_buffer_count = 1;
	queue_submit_info.command_buffers      = &cmd;
	queue_submit( iqueue, &queue_submit_info );
	queue_wait_idle( iqueue );
}

void
vk_queue_present( const Queue* iqueue, const QueuePresentInfo* info )
{
	FT_FROM_HANDLE( swapchain, info->swapchain, VulkanSwapchain );
	FT_FROM_HANDLE( queue, iqueue, VulkanQueue );

	std::vector<VkSemaphore> wait_semaphores( info->wait_semaphore_count );
	for ( u32 i = 0; i < info->wait_semaphore_count; ++i )
	{
		FT_FROM_HANDLE( semaphore,
		                info->wait_semaphores[ i ],
		                VulkanSemaphore );
		wait_semaphores[ i ] = semaphore->semaphore;
	}

	VkPresentInfoKHR present_info = {};

	present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pNext              = nullptr;
	present_info.waitSemaphoreCount = info->wait_semaphore_count;
	present_info.pWaitSemaphores    = wait_semaphores.data();
	present_info.swapchainCount     = 1;
	present_info.pSwapchains        = &swapchain->swapchain;
	present_info.pImageIndices      = &info->image_index;
	present_info.pResults           = nullptr;

	vkQueuePresentKHR( queue->queue, &present_info );
}

void
vk_create_semaphore( const Device* idevice, Semaphore** p )
{
	FT_ASSERT( p );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	FT_INIT_INTERNAL( semaphore, *p, VulkanSemaphore );

	VkSemaphoreCreateInfo semaphore_create_info {};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphore_create_info.pNext = nullptr;
	semaphore_create_info.flags = 0;

	VK_ASSERT( vkCreateSemaphore( device->logical_device,
	                              &semaphore_create_info,
	                              device->vulkan_allocator,
	                              &semaphore->semaphore ) );
}

void
vk_destroy_semaphore( const Device* idevice, Semaphore* isemaphore )
{
	FT_ASSERT( isemaphore );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( semaphore, isemaphore, VulkanSemaphore );

	vkDestroySemaphore( device->logical_device,
	                    semaphore->semaphore,
	                    device->vulkan_allocator );
	std::free( semaphore );
}

void
vk_create_fence( const Device* idevice, Fence** p )
{
	FT_ASSERT( p );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	FT_INIT_INTERNAL( fence, *p, VulkanFence );

	VkFenceCreateInfo fence_create_info {};
	fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_create_info.pNext = nullptr;
	fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VK_ASSERT( vkCreateFence( device->logical_device,
	                          &fence_create_info,
	                          device->vulkan_allocator,
	                          &fence->fence ) );
}

void
vk_destroy_fence( const Device* idevice, Fence* ifence )
{
	FT_ASSERT( ifence );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( fence, ifence, VulkanFence );

	vkDestroyFence( device->logical_device,
	                fence->fence,
	                device->vulkan_allocator );
	std::free( fence );
}

void
vk_wait_for_fences( const Device* idevice, u32 count, Fence** ifences )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	std::vector<VkFence> fences( count );
	for ( u32 i = 0; i < count; ++i )
	{
		FT_FROM_HANDLE( fence, ifences[ i ], VulkanFence );
		fences[ i ] = fence->fence;
	}

	vkWaitForFences( device->logical_device,
	                 count,
	                 fences.data(),
	                 true,
	                 std::numeric_limits<u64>::max() );
}

void
vk_reset_fences( const Device* idevice, u32 count, Fence** ifences )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	std::vector<VkFence> fences( count );
	for ( u32 i = 0; i < count; ++i )
	{
		FT_FROM_HANDLE( fence, ifences[ i ], VulkanFence );
		fences[ i ] = fence->fence;
	}

	vkResetFences( device->logical_device, count, fences.data() );
}

void
configure_swapchain( const VulkanDevice*  device,
                     VulkanSwapchain*     swapchain,
                     const SwapchainInfo* info )
{
	SDL_Vulkan_CreateSurface( ( SDL_Window* ) get_app_window()->handle,
	                          device->instance,
	                          &swapchain->surface );

	VkBool32 support_surface = false;
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
	                                           nullptr );
	std::vector<VkPresentModeKHR> present_modes( present_mode_count );
	vkGetPhysicalDeviceSurfacePresentModesKHR( device->physical_device,
	                                           swapchain->surface,
	                                           &present_mode_count,
	                                           present_modes.data() );

	swapchain->present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;

	VkPresentModeKHR preffered_mode =
	    info->vsync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_MAILBOX_KHR;

	for ( u32 i = 0; i < present_mode_count; ++i )
	{
		if ( present_modes[ i ] == preffered_mode )
		{
			swapchain->present_mode = preffered_mode;
			break;
		}
	}

	FT_INFO( "Swapchain present mode: {}",
	         string_VkPresentModeKHR( swapchain->present_mode ) );

	// determine present image count
	VkSurfaceCapabilitiesKHR surface_capabilities {};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device->physical_device,
	                                           swapchain->surface,
	                                           &surface_capabilities );

	// determine swapchain size
	swapchain->interface.width =
	    std::clamp( info->width,
	                surface_capabilities.minImageExtent.width,
	                surface_capabilities.maxImageExtent.width );
	swapchain->interface.height =
	    std::clamp( info->height,
	                surface_capabilities.minImageExtent.height,
	                surface_capabilities.maxImageExtent.height );

	swapchain->interface.min_image_count =
	    std::clamp( info->min_image_count,
	                surface_capabilities.minImageCount,
	                surface_capabilities.maxImageCount );

	/// find best surface format
	u32 surface_format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR( device->physical_device,
	                                      swapchain->surface,
	                                      &surface_format_count,
	                                      nullptr );
	std::vector<VkSurfaceFormatKHR> surface_formats( surface_format_count );
	vkGetPhysicalDeviceSurfaceFormatsKHR( device->physical_device,
	                                      swapchain->surface,
	                                      &surface_format_count,
	                                      surface_formats.data() );

	VkSurfaceFormatKHR surface_format   = surface_formats[ 0 ];
	VkFormat           preffered_format = to_vk_format( info->format );

	for ( u32 i = 0; i < surface_format_count; ++i )
	{
		if ( surface_formats[ i ].format == preffered_format )
			surface_format = surface_formats[ i ];
	}

	swapchain->interface.queue  = info->queue;
	swapchain->interface.format = from_vk_format( surface_format.format );
	swapchain->color_space      = surface_format.colorSpace;

	FT_INFO( "Swapchain surface format: {}",
	         string_VkFormat( surface_format.format ) );
	FT_INFO( "Swapchain color space: {}",
	         string_VkColorSpaceKHR( surface_format.colorSpace ) );

	/// fins swapchain pretransform
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

void
create_configured_swapchain( const VulkanDevice* device,
                             VulkanSwapchain*    swapchain,
                             b32                 resize )
{
	// destroy old resources if it is resize
	if ( resize )
	{
		for ( u32 i = 0; i < swapchain->interface.image_count; ++i )
		{
			FT_FROM_HANDLE( image,
			                swapchain->interface.images[ i ],
			                VulkanImage );
			vkDestroyImageView( device->logical_device,
			                    image->image_view,
			                    device->vulkan_allocator );
			std::free( image );
		}
	}

	// create new
	VkSwapchainCreateInfoKHR swapchain_create_info {};
	swapchain_create_info.sType   = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_create_info.pNext   = nullptr;
	swapchain_create_info.flags   = 0;
	swapchain_create_info.surface = swapchain->surface;
	swapchain_create_info.minImageCount = swapchain->interface.min_image_count;
	swapchain_create_info.imageFormat =
	    to_vk_format( swapchain->interface.format );
	swapchain_create_info.imageColorSpace    = swapchain->color_space;
	swapchain_create_info.imageExtent.width  = swapchain->interface.width;
	swapchain_create_info.imageExtent.height = swapchain->interface.height;
	swapchain_create_info.imageArrayLayers   = 1;
	swapchain_create_info.imageUsage =
	    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	swapchain_create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_create_info.queueFamilyIndexCount = 1;
	swapchain_create_info.pQueueFamilyIndices =
	    &swapchain->interface.queue->family_index;
	swapchain_create_info.preTransform = swapchain->pre_transform;
	// TODO: choose composite alpha according to caps
	swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_create_info.presentMode    = swapchain->present_mode;
	swapchain_create_info.clipped        = true;
	swapchain_create_info.oldSwapchain   = nullptr;

	VK_ASSERT( vkCreateSwapchainKHR( device->logical_device,
	                                 &swapchain_create_info,
	                                 device->vulkan_allocator,
	                                 &swapchain->swapchain ) );

	vkGetSwapchainImagesKHR( device->logical_device,
	                         swapchain->swapchain,
	                         &swapchain->interface.image_count,
	                         nullptr );
	std::vector<VkImage> swapchain_images( swapchain->interface.image_count );
	vkGetSwapchainImagesKHR( device->logical_device,
	                         swapchain->swapchain,
	                         &swapchain->interface.image_count,
	                         swapchain_images.data() );

	if ( !resize )
	{
		swapchain->interface.images = static_cast<Image**>(
		    std::calloc( swapchain->interface.image_count, sizeof( Image* ) ) );
	}

	VkImageViewCreateInfo image_view_create_info {};
	image_view_create_info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_create_info.pNext    = nullptr;
	image_view_create_info.flags    = 0;
	image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	image_view_create_info.format = to_vk_format( swapchain->interface.format );
	image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.subresourceRange.aspectMask =
	    VK_IMAGE_ASPECT_COLOR_BIT;
	image_view_create_info.subresourceRange.baseMipLevel   = 0;
	image_view_create_info.subresourceRange.levelCount     = 1;
	image_view_create_info.subresourceRange.baseArrayLayer = 0;
	image_view_create_info.subresourceRange.layerCount     = 1;

	for ( u32 i = 0; i < swapchain->interface.image_count; ++i )
	{
		FT_INIT_INTERNAL( image,
		                  swapchain->interface.images[ i ],
		                  VulkanImage );

		image_view_create_info.image = swapchain_images[ i ];

		image->image                     = swapchain_images[ i ];
		image->interface.width           = swapchain->interface.width;
		image->interface.height          = swapchain->interface.height;
		image->interface.format          = swapchain->interface.format;
		image->interface.sample_count    = SampleCount::e1;
		image->interface.mip_level_count = 1;
		image->interface.layer_count     = 1;
		image->interface.descriptor_type = DescriptorType::eSampledImage;

		VK_ASSERT( vkCreateImageView( device->logical_device,
		                              &image_view_create_info,
		                              device->vulkan_allocator,
		                              &image->image_view ) );
	}
}

void
vk_create_swapchain( const Device*        idevice,
                     const SwapchainInfo* info,
                     Swapchain**          p )
{
	FT_ASSERT( p );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	FT_INIT_INTERNAL( swapchain, *p, VulkanSwapchain );

	configure_swapchain( device, swapchain, info );
	create_configured_swapchain( device, swapchain, false );
}

void
vk_resize_swapchain( const Device* idevice,
                     Swapchain*    iswapchain,
                     u32           width,
                     u32           height )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( swapchain, iswapchain, VulkanSwapchain );

	iswapchain->width  = width;
	iswapchain->height = height;
	create_configured_swapchain( device, swapchain, true );
}

void
vk_destroy_swapchain( const Device* idevice, Swapchain* iswapchain )
{
	FT_ASSERT( iswapchain );
	FT_ASSERT( iswapchain->image_count );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( swapchain, iswapchain, VulkanSwapchain );

	for ( u32 i = 0; i < swapchain->interface.image_count; ++i )
	{
		FT_FROM_HANDLE( image, swapchain->interface.images[ i ], VulkanImage );

		vkDestroyImageView( device->logical_device,
		                    image->image_view,
		                    device->vulkan_allocator );
		std::free( image );
	}

	std::free( swapchain->interface.images );

	vkDestroySwapchainKHR( device->logical_device,
	                       swapchain->swapchain,
	                       device->vulkan_allocator );

	vkDestroySurfaceKHR( device->instance,
	                     swapchain->surface,
	                     device->vulkan_allocator );

	std::free( swapchain );
}

void
vk_create_command_pool( const Device*          idevice,
                        const CommandPoolInfo* info,
                        CommandPool**          p )
{
	FT_ASSERT( p );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	FT_INIT_INTERNAL( command_pool, *p, VulkanCommandPool );

	command_pool->interface.queue = info->queue;

	VkCommandPoolCreateInfo command_pool_create_info {};
	command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_create_info.pNext = nullptr;
	command_pool_create_info.flags =
	    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	command_pool_create_info.queueFamilyIndex = info->queue->family_index;

	VK_ASSERT( vkCreateCommandPool( device->logical_device,
	                                &command_pool_create_info,
	                                device->vulkan_allocator,
	                                &command_pool->command_pool ) );
}

void
vk_destroy_command_pool( const Device* idevice, CommandPool* icommand_pool )
{
	FT_ASSERT( icommand_pool );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( command_pool, icommand_pool, VulkanCommandPool );

	vkDestroyCommandPool( device->logical_device,
	                      command_pool->command_pool,
	                      device->vulkan_allocator );
	std::free( command_pool );
}

void
vk_create_command_buffers( const Device*      idevice,
                           const CommandPool* icommand_pool,
                           u32                count,
                           CommandBuffer**    icommand_buffers )
{
	FT_ASSERT( icommand_buffers );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( command_pool, icommand_pool, VulkanCommandPool );

	std::vector<VkCommandBuffer> buffers( count );

	VkCommandBufferAllocateInfo command_buffer_allocate_info {};
	command_buffer_allocate_info.sType =
	    VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_allocate_info.pNext       = nullptr;
	command_buffer_allocate_info.commandPool = command_pool->command_pool;
	command_buffer_allocate_info.level       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_allocate_info.commandBufferCount = count;

	VK_ASSERT( vkAllocateCommandBuffers( device->logical_device,
	                                     &command_buffer_allocate_info,
	                                     buffers.data() ) );

	// TODO: FIX ME!
	for ( u32 i = 0; i < count; ++i )
	{
		FT_INIT_INTERNAL( cmd, icommand_buffers[ i ], VulkanCommandBuffer );

		cmd->command_buffer  = buffers[ i ];
		cmd->interface.queue = command_pool->interface.queue;
	}
}

void
vk_free_command_buffers( const Device*      idevice,
                         const CommandPool* icommand_pool,
                         u32                count,
                         CommandBuffer**    icommand_buffers )
{
	FT_ASSERT( icommand_buffers );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	std::vector<VkCommandBuffer> buffers( count );

	for ( u32 i = 0; i < count; ++i )
	{
		FT_FROM_HANDLE( cmd, icommand_buffers[ i ], VulkanCommandBuffer );
		buffers[ i ] = cmd->command_buffer;
	}

	vkFreeCommandBuffers(
	    device->logical_device,
	    static_cast<VulkanCommandPool*>( icommand_pool->handle )->command_pool,
	    count,
	    buffers.data() );
}

void
vk_destroy_command_buffers( const Device*,
                            const CommandPool*,
                            u32             count,
                            CommandBuffer** icommand_buffers )
{
	FT_ASSERT( icommand_buffers );

	for ( u32 i = 0; i < count; ++i )
	{
		FT_FROM_HANDLE( cmd, icommand_buffers[ i ], VulkanCommandBuffer );
		std::free( cmd );
	}
}

void
vk_begin_command_buffer( const CommandBuffer* icmd )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );

	VkCommandBufferBeginInfo command_buffer_begin_info {};
	command_buffer_begin_info.sType =
	    VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	command_buffer_begin_info.pNext = nullptr;
	command_buffer_begin_info.flags =
	    VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	command_buffer_begin_info.pInheritanceInfo = nullptr;

	VK_ASSERT( vkBeginCommandBuffer( cmd->command_buffer,
	                                 &command_buffer_begin_info ) );
}

void
vk_end_command_buffer( const CommandBuffer* icmd )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );
	VK_ASSERT( vkEndCommandBuffer( cmd->command_buffer ) );
}

void
vk_acquire_next_image( const Device*    idevice,
                       const Swapchain* iswapchain,
                       const Semaphore* isemaphore,
                       const Fence*     ifence,
                       u32*             image_index )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( swapchain, iswapchain, VulkanSwapchain );
	FT_FROM_HANDLE( semaphore, isemaphore, VulkanSemaphore );

	VkResult result = vkAcquireNextImageKHR(
	    device->logical_device,
	    swapchain->swapchain,
	    std::numeric_limits<u64>::max(),
	    semaphore->semaphore,
	    ifence ? static_cast<VulkanFence*>( ifence->handle )->fence
	           : VK_NULL_HANDLE,
	    image_index );
	// TODO: check status
	( void ) result;
}

void
create_framebuffer( const VulkanDevice*   device,
                    VulkanRenderPass*     render_pass,
                    const RenderPassInfo* info )
{
	u32 attachment_count = info->color_attachment_count;

	VkImageView image_views[ MAX_ATTACHMENTS_COUNT + 2 ];
	for ( u32 i = 0; i < attachment_count; ++i )
	{
		FT_FROM_HANDLE( image, info->color_attachments[ i ], VulkanImage );
		image_views[ i ] = image->image_view;
	}

	if ( info->depth_stencil )
	{
		FT_FROM_HANDLE( image, info->depth_stencil, VulkanImage );
		image_views[ attachment_count++ ] = image->image_view;
	}

	if ( info->width > 0 && info->height > 0 )
	{
		VkFramebufferCreateInfo framebuffer_create_info {};
		framebuffer_create_info.sType =
		    VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_create_info.pNext           = nullptr;
		framebuffer_create_info.flags           = 0;
		framebuffer_create_info.renderPass      = render_pass->render_pass;
		framebuffer_create_info.attachmentCount = attachment_count;
		framebuffer_create_info.pAttachments    = image_views;
		framebuffer_create_info.width           = info->width;
		framebuffer_create_info.height          = info->height;
		framebuffer_create_info.layers          = 1;

		VK_ASSERT( vkCreateFramebuffer( device->logical_device,
		                                &framebuffer_create_info,
		                                device->vulkan_allocator,
		                                &render_pass->framebuffer ) );
	}
}

void
vk_create_render_pass( const Device*         idevice,
                       const RenderPassInfo* info,
                       RenderPass**          p )
{
	FT_ASSERT( p );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	FT_INIT_INTERNAL( render_pass, *p, VulkanRenderPass );

	render_pass->interface.color_attachment_count =
	    info->color_attachment_count;
	render_pass->interface.width  = info->width;
	render_pass->interface.height = info->height;
	render_pass->interface.sample_count =
	    info->color_attachments[ 0 ]->sample_count;

	u32 attachments_count = info->color_attachment_count;

	VkAttachmentDescription
	                      attachment_descriptions[ MAX_ATTACHMENTS_COUNT + 2 ];
	VkAttachmentReference color_attachment_references[ MAX_ATTACHMENTS_COUNT ];
	VkAttachmentReference depth_attachment_reference {};

	for ( u32 i = 0; i < info->color_attachment_count; ++i )
	{
		attachment_descriptions[ i ].flags = 0;
		attachment_descriptions[ i ].format =
		    to_vk_format( info->color_attachments[ i ]->format );
		attachment_descriptions[ i ].samples =
		    to_vk_sample_count( info->color_attachments[ i ]->sample_count );
		attachment_descriptions[ i ].loadOp =
		    to_vk_load_op( info->color_attachment_load_ops[ i ] );
		attachment_descriptions[ i ].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachment_descriptions[ i ].stencilLoadOp =
		    VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment_descriptions[ i ].stencilStoreOp =
		    VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment_descriptions[ i ].initialLayout =
		    determine_image_layout( info->color_image_states[ i ] );
		attachment_descriptions[ i ].finalLayout =
		    determine_image_layout( info->color_image_states[ i ] );

		color_attachment_references[ i ].attachment = i;
		color_attachment_references[ i ].layout =
		    determine_image_layout( info->color_image_states[ i ] );
	}

	if ( info->depth_stencil )
	{
		render_pass->interface.has_depth_stencil = true;

		u32 i                              = attachments_count;
		attachment_descriptions[ i ].flags = 0;
		attachment_descriptions[ i ].format =
		    to_vk_format( info->depth_stencil->format );
		attachment_descriptions[ i ].samples =
		    to_vk_sample_count( info->depth_stencil->sample_count );
		attachment_descriptions[ i ].loadOp =
		    to_vk_load_op( info->depth_stencil_load_op );
		attachment_descriptions[ i ].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment_descriptions[ i ].stencilLoadOp =
		    VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment_descriptions[ i ].stencilStoreOp =
		    VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment_descriptions[ i ].initialLayout =
		    determine_image_layout( info->depth_stencil_state );
		attachment_descriptions[ i ].finalLayout =
		    determine_image_layout( info->depth_stencil_state );

		depth_attachment_reference.attachment = i;
		depth_attachment_reference.layout =
		    attachment_descriptions[ i ].finalLayout;

		attachments_count++;
	}

	// TODO: subpass setup from user code
	VkSubpassDescription subpass_description {};
	subpass_description.flags                = 0;
	subpass_description.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass_description.inputAttachmentCount = 0;
	subpass_description.pInputAttachments    = nullptr;
	subpass_description.colorAttachmentCount = info->color_attachment_count;
	subpass_description.pColorAttachments =
	    attachments_count ? color_attachment_references : nullptr;
	subpass_description.pDepthStencilAttachment =
	    info->depth_stencil ? &depth_attachment_reference : nullptr;
	subpass_description.pResolveAttachments     = nullptr;
	subpass_description.preserveAttachmentCount = 0;
	subpass_description.pPreserveAttachments    = nullptr;

	VkRenderPassCreateInfo render_pass_create_info {};
	render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.pNext = nullptr;
	render_pass_create_info.flags = 0;
	render_pass_create_info.attachmentCount = attachments_count;
	render_pass_create_info.pAttachments =
	    attachments_count ? attachment_descriptions : nullptr;
	render_pass_create_info.subpassCount    = 1;
	render_pass_create_info.pSubpasses      = &subpass_description;
	render_pass_create_info.dependencyCount = 0;
	render_pass_create_info.pDependencies   = nullptr;

	VK_ASSERT( vkCreateRenderPass( device->logical_device,
	                               &render_pass_create_info,
	                               device->vulkan_allocator,
	                               &render_pass->render_pass ) );

	create_framebuffer( device, render_pass, info );
}

void
vk_resize_render_pass( const Device*         idevice,
                       RenderPass*           irender_pass,
                       const RenderPassInfo* info )
{
	FT_ASSERT( irender_pass );
	FT_ASSERT( info->width > 0 && info->height > 0 );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( render_pass, irender_pass, VulkanRenderPass );

	render_pass->interface.width  = info->width;
	render_pass->interface.height = info->height;

	vkDestroyFramebuffer( device->logical_device,
	                      render_pass->framebuffer,
	                      device->vulkan_allocator );

	create_framebuffer( device, render_pass, info );
}

void
vk_destroy_render_pass( const Device* idevice, RenderPass* irender_pass )
{
	FT_ASSERT( irender_pass );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( render_pass, irender_pass, VulkanRenderPass );

	if ( render_pass->framebuffer )
	{
		vkDestroyFramebuffer( device->logical_device,
		                      render_pass->framebuffer,
		                      device->vulkan_allocator );
	}
	vkDestroyRenderPass( device->logical_device,
	                     render_pass->render_pass,
	                     device->vulkan_allocator );
	std::free( render_pass );
}

void
vk_create_shader( const Device* idevice, ShaderInfo* info, Shader** p )
{
	FT_ASSERT( p );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	FT_INIT_INTERNAL( shader, *p, VulkanShader );

	auto create_module = []( const VulkanDevice*     device,
	                         VulkanShader*           shader,
	                         ShaderStage             stage,
	                         const ShaderModuleInfo& info )
	{
		if ( info.bytecode )
		{
			VkShaderModuleCreateInfo shader_create_info {};
			shader_create_info.sType =
			    VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			shader_create_info.pNext    = nullptr;
			shader_create_info.flags    = 0;
			shader_create_info.codeSize = info.bytecode_size;
			shader_create_info.pCode =
			    reinterpret_cast<const u32*>( info.bytecode );

			VK_ASSERT( vkCreateShaderModule(
			    device->logical_device,
			    &shader_create_info,
			    device->vulkan_allocator,
			    &shader->shaders[ static_cast<u32>( stage ) ] ) );
		}
	};

	create_module( device, shader, ShaderStage::eCompute, info->compute );
	create_module( device, shader, ShaderStage::eVertex, info->vertex );
	create_module( device,
	               shader,
	               ShaderStage::eTessellationControl,
	               info->tessellation_control );
	create_module( device,
	               shader,
	               ShaderStage::eTessellationEvaluation,
	               info->tessellation_evaluation );
	create_module( device, shader, ShaderStage::eGeometry, info->geometry );
	create_module( device, shader, ShaderStage::eFragment, info->fragment );

	spirv_reflect( idevice, info, &shader->interface );
}

void
vk_destroy_shader( const Device* idevice, Shader* ishader )
{
	FT_ASSERT( ishader );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( shader, ishader, VulkanShader );

	auto destroy_module = []( const VulkanDevice* device,
	                          ShaderStage         stage,
	                          VulkanShader*       shader )
	{
		if ( shader->shaders[ static_cast<u32>( stage ) ] )
		{
			vkDestroyShaderModule( device->logical_device,
			                       shader->shaders[ static_cast<u32>( stage ) ],
			                       device->vulkan_allocator );
		}
	};

	destroy_module( device, ShaderStage::eCompute, shader );
	destroy_module( device, ShaderStage::eVertex, shader );
	destroy_module( device, ShaderStage::eTessellationControl, shader );
	destroy_module( device, ShaderStage::eTessellationEvaluation, shader );
	destroy_module( device, ShaderStage::eGeometry, shader );
	destroy_module( device, ShaderStage::eFragment, shader );

	std::free( shader );
}

void
vk_create_descriptor_set_layout( const Device*         idevice,
                                 Shader*               ishader,
                                 DescriptorSetLayout** p )
{
	FT_ASSERT( p );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	FT_INIT_INTERNAL( descriptor_set_layout, *p, VulkanDescriptorSetLayout );

	descriptor_set_layout->interface.shader = ishader;

	// count bindings in all shaders
	u32                      binding_counts[ MAX_SET_COUNT ] = { 0 };
	VkDescriptorBindingFlags binding_flags[ MAX_SET_COUNT ]
	                                      [ MAX_DESCRIPTOR_BINDING_COUNT ] = {};
	// collect all bindings
	VkDescriptorSetLayoutBinding bindings[ MAX_SET_COUNT ]
	                                     [ MAX_DESCRIPTOR_BINDING_COUNT ];

	u32 set_count = 0;

	for ( u32 i = 0; i < static_cast<u32>( ShaderStage::eCount ); ++i )
	{
		for ( u32 j = 0; j < ishader->reflect_data[ i ].binding_count; ++j )
		{
			auto& binding       = ishader->reflect_data[ i ].bindings[ j ];
			u32   set           = binding.set;
			u32&  binding_count = binding_counts[ set ];

			bindings[ set ][ binding_count ].binding = binding.binding;
			bindings[ set ][ binding_count ].descriptorCount =
			    binding.descriptor_count;
			bindings[ set ][ binding_count ].descriptorType =
			    to_vk_descriptor_type( binding.descriptor_type );
			bindings[ set ][ binding_count ].pImmutableSamplers =
			    nullptr; // ??? TODO
			bindings[ set ][ binding_count ].stageFlags =
			    to_vk_shader_stage( static_cast<ShaderStage>( i ) );

			if ( binding.descriptor_count > 1 )
			{
				binding_flags[ set ][ binding_count ] =
				    VkDescriptorBindingFlags {};
				binding_flags[ set ][ binding_count ] |=
				    VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
			}

			binding_count++;

			if ( set + 1 > set_count )
			{
				set_count = set + 1;
			}
		}
	}

	for ( u32 set = 0; set < set_count; ++set )
	{
		if ( binding_counts[ set ] > 0 )
		{
			VkDescriptorSetLayoutBindingFlagsCreateInfo
			    binding_flags_create_info {};
			binding_flags_create_info.sType =
			    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
			binding_flags_create_info.pNext         = nullptr;
			binding_flags_create_info.bindingCount  = binding_counts[ set ];
			binding_flags_create_info.pBindingFlags = binding_flags[ set ];

			VkDescriptorSetLayoutCreateInfo
			    descriptor_set_layout_create_info {};
			descriptor_set_layout_create_info.sType =
			    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptor_set_layout_create_info.pNext =
			    &binding_flags_create_info;
			descriptor_set_layout_create_info.flags = 0;
			descriptor_set_layout_create_info.bindingCount =
			    binding_counts[ set ];
			descriptor_set_layout_create_info.pBindings = bindings[ set ];

			VK_ASSERT( vkCreateDescriptorSetLayout(
			    device->logical_device,
			    &descriptor_set_layout_create_info,
			    device->vulkan_allocator,
			    &descriptor_set_layout->descriptor_set_layouts[ set ] ) );

			descriptor_set_layout->interface.descriptor_set_layout_count++;
		}
	}
}

void
vk_destroy_descriptor_set_layout( const Device*        idevice,
                                  DescriptorSetLayout* ilayout )
{
	FT_ASSERT( ilayout );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( layout, ilayout, VulkanDescriptorSetLayout );

	for ( u32 i = 0; i < layout->interface.descriptor_set_layout_count; ++i )
	{
		if ( layout->descriptor_set_layouts[ i ] )
		{
			vkDestroyDescriptorSetLayout( device->logical_device,
			                              layout->descriptor_set_layouts[ i ],
			                              device->vulkan_allocator );
		}
	}

	std::free( layout );
}

void
vk_create_compute_pipeline( const Device*       idevice,
                            const PipelineInfo* info,
                            Pipeline**          p )
{
	FT_ASSERT( p );
	FT_ASSERT( info->descriptor_set_layout );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( shader, info->shader, VulkanShader );

	FT_INIT_INTERNAL( pipeline, *p, VulkanPipeline );

	pipeline->interface.type = PipelineType::eCompute;

	VkPipelineShaderStageCreateInfo shader_stage_create_info;
	shader_stage_create_info.sType =
	    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage_create_info.pNext = nullptr;
	shader_stage_create_info.flags = 0;
	shader_stage_create_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	shader_stage_create_info.module =
	    shader->shaders[ static_cast<u32>( ShaderStage::eCompute ) ];
	shader_stage_create_info.pName               = "main";
	shader_stage_create_info.pSpecializationInfo = nullptr;

	VkPushConstantRange push_constant_range {};
	push_constant_range.size       = MAX_PUSH_CONSTANT_RANGE;
	push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
	                                 VK_SHADER_STAGE_COMPUTE_BIT |
	                                 VK_SHADER_STAGE_FRAGMENT_BIT;

	VkPipelineLayoutCreateInfo pipeline_layout_create_info {};
	pipeline_layout_create_info.sType =
	    VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_create_info.setLayoutCount =
	    info->descriptor_set_layout->descriptor_set_layout_count;
	pipeline_layout_create_info.pSetLayouts =
	    static_cast<VulkanDescriptorSetLayout*>(
	        info->descriptor_set_layout->handle )
	        ->descriptor_set_layouts;
	pipeline_layout_create_info.pushConstantRangeCount = 1;
	pipeline_layout_create_info.pPushConstantRanges    = &push_constant_range;

	VK_ASSERT( vkCreatePipelineLayout( device->logical_device,
	                                   &pipeline_layout_create_info,
	                                   nullptr,
	                                   &pipeline->pipeline_layout ) );

	VkComputePipelineCreateInfo compute_pipeline_create_info {};
	compute_pipeline_create_info.sType =
	    VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	compute_pipeline_create_info.stage  = shader_stage_create_info;
	compute_pipeline_create_info.layout = pipeline->pipeline_layout;

	VK_ASSERT( vkCreateComputePipelines( device->logical_device,
	                                     VK_NULL_HANDLE,
	                                     1,
	                                     &compute_pipeline_create_info,
	                                     device->vulkan_allocator,
	                                     &pipeline->pipeline ) );
}

void
vk_create_graphics_pipeline( const Device*       idevice,
                             const PipelineInfo* info,
                             Pipeline**          p )
{
	FT_ASSERT( p );
	FT_ASSERT( info->descriptor_set_layout );
	FT_ASSERT( info->render_pass );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( shader, info->shader, VulkanShader );

	FT_INIT_INTERNAL( pipeline, *p, VulkanPipeline );

	pipeline->interface.type = PipelineType::eGraphics;

	u32 shader_stage_count = 0;
	VkPipelineShaderStageCreateInfo
	    shader_stage_create_infos[ static_cast<u32>( ShaderStage::eCount ) ];

	for ( u32 i = 0; i < static_cast<u32>( ShaderStage::eCount ); ++i )
	{
		if ( shader->shaders[ i ] == VK_NULL_HANDLE )
		{
			continue;
		}

		shader_stage_create_infos[ shader_stage_count ].sType =
		    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stage_create_infos[ shader_stage_count ].pNext = nullptr;
		shader_stage_create_infos[ shader_stage_count ].flags = 0;
		shader_stage_create_infos[ shader_stage_count ].stage =
		    to_vk_shader_stage( static_cast<ShaderStage>( i ) );
		shader_stage_create_infos[ shader_stage_count ].module =
		    shader->shaders[ i ];
		shader_stage_create_infos[ shader_stage_count ].pName = "main";
		shader_stage_create_infos[ shader_stage_count ].pSpecializationInfo =
		    nullptr;
		shader_stage_count++;
	}

	FT_ASSERT( shader_stage_count > 0 );

	const VertexLayout& vertex_layout = info->vertex_layout;

	VkVertexInputBindingDescription
	    binding_descriptions[ MAX_VERTEX_BINDING_COUNT ];
	for ( u32 i = 0; i < vertex_layout.binding_info_count; ++i )
	{
		binding_descriptions[ i ].binding =
		    vertex_layout.binding_infos[ i ].binding;
		binding_descriptions[ i ].stride =
		    vertex_layout.binding_infos[ i ].stride;
		binding_descriptions[ i ].inputRate = to_vk_vertex_input_rate(
		    vertex_layout.binding_infos[ i ].input_rate );
	}

	VkVertexInputAttributeDescription
	    attribute_descriptions[ MAX_VERTEX_ATTRIBUTE_COUNT ];
	for ( u32 i = 0; i < vertex_layout.attribute_info_count; ++i )
	{
		attribute_descriptions[ i ].location =
		    vertex_layout.attribute_infos[ i ].location;
		attribute_descriptions[ i ].binding =
		    vertex_layout.attribute_infos[ i ].binding;
		attribute_descriptions[ i ].format =
		    to_vk_format( vertex_layout.attribute_infos[ i ].format );
		attribute_descriptions[ i ].offset =
		    vertex_layout.attribute_infos[ i ].offset;
	}

	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info {};
	vertex_input_state_create_info.sType =
	    VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_state_create_info.vertexBindingDescriptionCount =
	    vertex_layout.binding_info_count;
	vertex_input_state_create_info.pVertexBindingDescriptions =
	    binding_descriptions;
	vertex_input_state_create_info.vertexAttributeDescriptionCount =
	    vertex_layout.attribute_info_count;
	vertex_input_state_create_info.pVertexAttributeDescriptions =
	    attribute_descriptions;

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info {};
	input_assembly_state_create_info.sType =
	    VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_state_create_info.topology =
	    to_vk_primitive_topology( info->topology );
	input_assembly_state_create_info.primitiveRestartEnable = false;

	// Dynamic states
	VkViewport viewport {};
	VkRect2D   scissor {};

	VkPipelineViewportStateCreateInfo viewport_state_create_info {};
	viewport_state_create_info.sType =
	    VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state_create_info.viewportCount = 1;
	viewport_state_create_info.pViewports    = &viewport;
	viewport_state_create_info.scissorCount  = 1;
	viewport_state_create_info.pScissors     = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterization_state_create_info {};
	rasterization_state_create_info.sType =
	    VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_state_create_info.polygonMode =
	    to_vk_polygon_mode( info->rasterizer_info.polygon_mode );
	rasterization_state_create_info.cullMode =
	    to_vk_cull_mode( info->rasterizer_info.cull_mode );
	rasterization_state_create_info.frontFace =
	    to_vk_front_face( info->rasterizer_info.front_face );
	rasterization_state_create_info.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisample_state_create_info {};
	multisample_state_create_info.sType =
	    VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_state_create_info.rasterizationSamples =
	    to_vk_sample_count( info->render_pass->sample_count );
	multisample_state_create_info.sampleShadingEnable = VK_FALSE;
	multisample_state_create_info.minSampleShading    = 1.0f;

	std::vector<VkPipelineColorBlendAttachmentState> attachment_states(
	    info->render_pass->color_attachment_count );
	for ( u32 i = 0; i < attachment_states.size(); ++i )
	{
		attachment_states[ i ]                     = {};
		attachment_states[ i ].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		attachment_states[ i ].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		attachment_states[ i ].colorBlendOp        = VK_BLEND_OP_ADD;
		attachment_states[ i ].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		attachment_states[ i ].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		attachment_states[ i ].colorWriteMask =
		    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	}

	VkPipelineColorBlendStateCreateInfo color_blend_state_create_info {};
	color_blend_state_create_info.sType =
	    VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_state_create_info.logicOpEnable = false;
	color_blend_state_create_info.logicOp       = VK_LOGIC_OP_COPY;
	color_blend_state_create_info.attachmentCount =
	    info->render_pass->color_attachment_count;
	color_blend_state_create_info.pAttachments = attachment_states.data();

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info {};
	depth_stencil_state_create_info.sType =
	    VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil_state_create_info.depthTestEnable =
	    info->depth_state_info.depth_test ? VK_TRUE : VK_FALSE;
	depth_stencil_state_create_info.depthWriteEnable =
	    info->depth_state_info.depth_write ? VK_TRUE : VK_FALSE;
	depth_stencil_state_create_info.depthCompareOp =
	    info->depth_state_info.depth_test
	        ? to_vk_compare_op( info->depth_state_info.compare_op )
	        : VK_COMPARE_OP_ALWAYS;
	depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
	depth_stencil_state_create_info.minDepthBounds        = 0.0f; // Optional
	depth_stencil_state_create_info.maxDepthBounds        = 1.0f; // Optional
	depth_stencil_state_create_info.stencilTestEnable     = VK_FALSE;

	const uint32_t dynamic_state_count                   = 2;
	VkDynamicState dynamic_states[ dynamic_state_count ] = {
	    VK_DYNAMIC_STATE_SCISSOR,
	    VK_DYNAMIC_STATE_VIEWPORT
	};

	VkPipelineDynamicStateCreateInfo dynamic_state_create_info {};
	dynamic_state_create_info.sType =
	    VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_create_info.dynamicStateCount = dynamic_state_count;
	dynamic_state_create_info.pDynamicStates    = dynamic_states;

	VkPushConstantRange push_constant_range {};
	push_constant_range.size       = MAX_PUSH_CONSTANT_RANGE;
	push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
	                                 VK_SHADER_STAGE_COMPUTE_BIT |
	                                 VK_SHADER_STAGE_FRAGMENT_BIT;

	VkPipelineLayoutCreateInfo pipeline_layout_create_info {};
	pipeline_layout_create_info.sType =
	    VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_create_info.setLayoutCount =
	    info->descriptor_set_layout->descriptor_set_layout_count;
	pipeline_layout_create_info.pSetLayouts =
	    static_cast<VulkanDescriptorSetLayout*>(
	        info->descriptor_set_layout->handle )
	        ->descriptor_set_layouts;
	pipeline_layout_create_info.pushConstantRangeCount = 1;
	pipeline_layout_create_info.pPushConstantRanges    = &push_constant_range;

	VK_ASSERT( vkCreatePipelineLayout( device->logical_device,
	                                   &pipeline_layout_create_info,
	                                   device->vulkan_allocator,
	                                   &pipeline->pipeline_layout ) );

	VkGraphicsPipelineCreateInfo pipeline_create_info {};
	pipeline_create_info.sType =
	    VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_create_info.stageCount        = shader_stage_count;
	pipeline_create_info.pStages           = shader_stage_create_infos;
	pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
	pipeline_create_info.pInputAssemblyState =
	    &input_assembly_state_create_info;
	pipeline_create_info.pViewportState      = &viewport_state_create_info;
	pipeline_create_info.pMultisampleState   = &multisample_state_create_info;
	pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
	pipeline_create_info.pColorBlendState    = &color_blend_state_create_info;
	pipeline_create_info.pDepthStencilState  = &depth_stencil_state_create_info;
	pipeline_create_info.pDynamicState       = &dynamic_state_create_info;
	pipeline_create_info.layout              = pipeline->pipeline_layout;
	pipeline_create_info.renderPass =
	    static_cast<VulkanRenderPass*>( info->render_pass->handle )
	        ->render_pass;

	VK_ASSERT( vkCreateGraphicsPipelines( device->logical_device,
	                                      VK_NULL_HANDLE,
	                                      1,
	                                      &pipeline_create_info,
	                                      device->vulkan_allocator,
	                                      &pipeline->pipeline ) );
}

void
vk_destroy_pipeline( const Device* idevice, Pipeline* ipipeline )
{
	FT_ASSERT( ipipeline );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( pipeline, ipipeline, VulkanPipeline );

	vkDestroyPipelineLayout( device->logical_device,
	                         pipeline->pipeline_layout,
	                         device->vulkan_allocator );
	vkDestroyPipeline( device->logical_device,
	                   pipeline->pipeline,
	                   device->vulkan_allocator );
	std::free( pipeline );
}

void
vk_create_buffer( const Device* idevice, const BufferInfo* info, Buffer** p )
{
	FT_ASSERT( p );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	FT_INIT_INTERNAL( buffer, *p, VulkanBuffer );

	buffer->interface.size            = info->size;
	buffer->interface.descriptor_type = info->descriptor_type;
	buffer->interface.memory_usage    = info->memory_usage;

	VmaAllocationCreateInfo allocation_create_info {};
	allocation_create_info.usage =
	    determine_vma_memory_usage( info->memory_usage );

	VkBufferCreateInfo buffer_create_info {};
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.pNext = nullptr;
	buffer_create_info.flags = 0;
	buffer_create_info.size  = info->size;
	buffer_create_info.usage =
	    determine_vk_buffer_usage( info->descriptor_type, info->memory_usage );
	buffer_create_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
	buffer_create_info.queueFamilyIndexCount = 0;
	buffer_create_info.pQueueFamilyIndices   = nullptr;

	VK_ASSERT( vmaCreateBuffer( device->memory_allocator,
	                            &buffer_create_info,
	                            &allocation_create_info,
	                            &buffer->buffer,
	                            &buffer->allocation,
	                            nullptr ) );
}

void
vk_destroy_buffer( const Device* idevice, Buffer* ibuffer )
{
	FT_ASSERT( ibuffer );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( buffer, ibuffer, VulkanBuffer );

	vmaDestroyBuffer( device->memory_allocator,
	                  buffer->buffer,
	                  buffer->allocation );
	std::free( buffer );
}

void
vk_create_sampler( const Device* idevice, const SamplerInfo* info, Sampler** p )
{
	FT_ASSERT( p );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	FT_INIT_INTERNAL( sampler, *p, VulkanSampler );

	VkSamplerCreateInfo sampler_create_info {};
	sampler_create_info.sType     = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_create_info.pNext     = nullptr;
	sampler_create_info.flags     = 0;
	sampler_create_info.magFilter = to_vk_filter( info->mag_filter );
	sampler_create_info.minFilter = to_vk_filter( info->min_filter );
	sampler_create_info.mipmapMode =
	    to_vk_sampler_mipmap_mode( info->mipmap_mode );
	sampler_create_info.addressModeU =
	    to_vk_sampler_address_mode( info->address_mode_u );
	sampler_create_info.addressModeV =
	    to_vk_sampler_address_mode( info->address_mode_v );
	sampler_create_info.addressModeW =
	    to_vk_sampler_address_mode( info->address_mode_w );
	sampler_create_info.mipLodBias       = info->mip_lod_bias;
	sampler_create_info.anisotropyEnable = info->anisotropy_enable;
	sampler_create_info.maxAnisotropy    = info->max_anisotropy;
	sampler_create_info.compareEnable    = info->compare_enable;
	sampler_create_info.compareOp        = to_vk_compare_op( info->compare_op );
	sampler_create_info.minLod           = info->min_lod;
	sampler_create_info.maxLod           = info->max_lod;
	sampler_create_info.unnormalizedCoordinates = false;

	VK_ASSERT( vkCreateSampler( device->logical_device,
	                            &sampler_create_info,
	                            device->vulkan_allocator,
	                            &sampler->sampler ) );
}

void
vk_destroy_sampler( const Device* idevice, Sampler* isampler )
{
	FT_ASSERT( isampler );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( sampler, isampler, VulkanSampler );

	vkDestroySampler( device->logical_device,
	                  sampler->sampler,
	                  device->vulkan_allocator );
	std::free( sampler );
}

void
vk_create_image( const Device* idevice, const ImageInfo* info, Image** p )
{
	FT_ASSERT( p );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	FT_INIT_INTERNAL( image, *p, VulkanImage );

	VmaAllocationCreateInfo allocation_create_info {};
	allocation_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	VkImageCreateInfo image_create_info {};
	image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.pNext = nullptr;
	image_create_info.flags = 0;
	// TODO: determine image type properly
	image_create_info.imageType     = VK_IMAGE_TYPE_2D;
	image_create_info.format        = to_vk_format( info->format );
	image_create_info.extent.width  = info->width;
	image_create_info.extent.height = info->height;
	image_create_info.extent.depth  = info->depth;
	image_create_info.mipLevels     = info->mip_levels;
	image_create_info.arrayLayers   = info->layer_count;
	image_create_info.samples       = to_vk_sample_count( info->sample_count );
	image_create_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
	image_create_info.usage = determine_vk_image_usage( info->descriptor_type );
	image_create_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
	image_create_info.queueFamilyIndexCount = 0;
	image_create_info.pQueueFamilyIndices   = nullptr;
	image_create_info.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;

	if ( info->layer_count == 6 )
	{
		image_create_info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}

	VK_ASSERT( vmaCreateImage( device->memory_allocator,
	                           &image_create_info,
	                           &allocation_create_info,
	                           &image->image,
	                           &image->allocation,
	                           nullptr ) );

	image->interface.width           = info->width;
	image->interface.height          = info->height;
	image->interface.depth           = info->depth;
	image->interface.format          = info->format;
	image->interface.sample_count    = info->sample_count;
	image->interface.mip_level_count = info->mip_levels;
	image->interface.layer_count     = info->layer_count;
	image->interface.descriptor_type = info->descriptor_type;

	// TODO: fill properly
	VkImageViewCreateInfo image_view_create_info {};
	image_view_create_info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_create_info.pNext    = nullptr;
	image_view_create_info.flags    = 0;
	image_view_create_info.image    = image->image;
	image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	if ( image->interface.layer_count == 6 )
	{
		image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	}
	image_view_create_info.format       = image_create_info.format;
	image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.subresourceRange =
	    get_image_subresource_range( image );

	VK_ASSERT( vkCreateImageView( device->logical_device,
	                              &image_view_create_info,
	                              device->vulkan_allocator,
	                              &image->image_view ) );
}

void
vk_destroy_image( const Device* idevice, Image* iimage )
{
	FT_ASSERT( iimage );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( image, iimage, VulkanImage );

	vkDestroyImageView( device->logical_device,
	                    image->image_view,
	                    device->vulkan_allocator );
	vmaDestroyImage( device->memory_allocator,
	                 image->image,
	                 image->allocation );
	std::free( image );
}

void
vk_create_descriptor_set( const Device*            idevice,
                          const DescriptorSetInfo* info,
                          DescriptorSet**          p )
{
	FT_ASSERT( p );
	FT_ASSERT( info->descriptor_set_layout );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	FT_INIT_INTERNAL( descriptor_set, *p, VulkanDescriptorSet );

	VkDescriptorSetAllocateInfo descriptor_set_allocate_info {};
	descriptor_set_allocate_info.sType =
	    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptor_set_allocate_info.pNext              = nullptr;
	descriptor_set_allocate_info.descriptorPool     = device->descriptor_pool;
	descriptor_set_allocate_info.descriptorSetCount = 1;
	descriptor_set_allocate_info.pSetLayouts =
	    &static_cast<VulkanDescriptorSetLayout*>(
	         info->descriptor_set_layout->handle )
	         ->descriptor_set_layouts[ info->set ];

	VK_ASSERT( vkAllocateDescriptorSets( device->logical_device,
	                                     &descriptor_set_allocate_info,
	                                     &descriptor_set->descriptor_set ) );
}

void
vk_destroy_descriptor_set( const Device* idevice, DescriptorSet* iset )
{
	FT_ASSERT( iset );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( set, iset, VulkanDescriptorSet );

	vkFreeDescriptorSets( device->logical_device,
	                      device->descriptor_pool,
	                      1,
	                      &set->descriptor_set );
	std::free( set );
}

void
vk_update_descriptor_set( const Device*          idevice,
                          DescriptorSet*         iset,
                          u32                    count,
                          const DescriptorWrite* writes )
{
	FT_ASSERT( iset );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( set, iset, VulkanDescriptorSet );

	// TODO: rewrite
	std::vector<std::vector<VkDescriptorBufferInfo>> buffer_updates( count );
	std::vector<std::vector<VkDescriptorImageInfo>>  image_updates( count );
	std::vector<VkWriteDescriptorSet>                descriptor_writes( count );

	u32 write = 0;

	for ( u32 i = 0; i < count; ++i )
	{
		const auto& descriptor_write = writes[ i ];

		auto& write_descriptor_set = descriptor_writes[ write++ ];
		write_descriptor_set       = {};
		write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_descriptor_set.dstBinding = descriptor_write.binding;
		write_descriptor_set.descriptorCount =
		    descriptor_write.descriptor_count;
		write_descriptor_set.dstSet = set->descriptor_set;
		write_descriptor_set.descriptorType =
		    to_vk_descriptor_type( descriptor_write.descriptor_type );

		if ( descriptor_write.buffer_descriptors )
		{
			auto& bds = buffer_updates.emplace_back();
			bds.resize( descriptor_write.descriptor_count );

			for ( u32 j = 0; j < descriptor_write.descriptor_count; ++j )
			{
				bds[ j ]        = {};
				bds[ j ].buffer = static_cast<VulkanBuffer*>(
				                      descriptor_write.buffer_descriptors[ j ]
				                          .buffer->handle )
				                      ->buffer;
				bds[ j ].offset =
				    descriptor_write.buffer_descriptors[ j ].offset;
				bds[ j ].range = descriptor_write.buffer_descriptors[ j ].range;
			}

			write_descriptor_set.pBufferInfo = bds.data();
		}
		else if ( descriptor_write.image_descriptors )
		{
			auto& ids = image_updates.emplace_back();
			ids.resize( descriptor_write.descriptor_count );

			for ( u32 j = 0; j < descriptor_write.descriptor_count; ++j )
			{
				auto& image_write = descriptor_write.image_descriptors[ j ];
				ids[ j ]          = {};

				if ( image_write.image )
				{
					ids[ j ].imageLayout =
					    determine_image_layout( image_write.resource_state );
					ids[ j ].imageView =
					    static_cast<VulkanImage*>( image_write.image->handle )
					        ->image_view;
					ids[ j ].sampler = nullptr;
				}
				else
				{
					FT_ASSERT( false && "Null descriptor" );
				}
			}

			write_descriptor_set.pImageInfo = ids.data();
		}
		else
		{
			auto& ids = image_updates.emplace_back();
			ids.resize( descriptor_write.descriptor_count );

			for ( u32 j = 0; j < descriptor_write.descriptor_count; ++j )
			{
				auto& image_write = descriptor_write.sampler_descriptors[ j ];
				ids[ j ]          = {};

				if ( image_write.sampler )
				{
					ids[ j ].sampler = static_cast<VulkanSampler*>(
					                       image_write.sampler->handle )
					                       ->sampler;
				}
				else
				{
					FT_ASSERT( false && "Null descriptor" );
				}
			}

			write_descriptor_set.pImageInfo = ids.data();
		}
	}

	vkUpdateDescriptorSets( device->logical_device,
	                        descriptor_writes.size(),
	                        descriptor_writes.data(),
	                        0,
	                        nullptr );
}

void
vk_create_ui_context( CommandBuffer* cmd, const UiInfo* info, UiContext** p )
{
	FT_ASSERT( p );

	FT_FROM_HANDLE( backend, info->backend, VulkanRendererBackend );
	FT_FROM_HANDLE( device, info->device, VulkanDevice );
	FT_FROM_HANDLE( queue, info->queue, VulkanQueue );
	FT_FROM_HANDLE( render_pass, info->render_pass, VulkanRenderPass );

	FT_INIT_INTERNAL( context, *p, VulkanUiContext );

	VkDescriptorPoolSize pool_sizes[] = {
	    { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
	    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
	    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
	    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
	    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
	    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
	    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
	    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
	    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
	    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
	    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets       = 1000;
	pool_info.poolSizeCount = std::size( pool_sizes );
	pool_info.pPoolSizes    = pool_sizes;

	VK_ASSERT( vkCreateDescriptorPool( device->logical_device,
	                                   &pool_info,
	                                   nullptr,
	                                   &context->desriptor_pool ) );

	ImGui::CreateContext();
	auto& io = ImGui::GetIO();
	( void ) io;
	if ( info->docking )
	{
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	}

	if ( info->viewports )
	{
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	}

	ImGui_ImplVulkan_InitInfo init_info {};
	init_info.Instance        = backend->instance;
	init_info.PhysicalDevice  = device->physical_device;
	init_info.Device          = device->logical_device;
	init_info.QueueFamily     = info->queue->family_index;
	init_info.Queue           = queue->queue;
	init_info.PipelineCache   = VkPipelineCache {};
	init_info.Allocator       = device->vulkan_allocator;
	init_info.MinImageCount   = info->min_image_count;
	init_info.ImageCount      = info->image_count;
	init_info.InFlyFrameCount = info->in_fly_frame_count;
	init_info.CheckVkResultFn = nullptr;
	init_info.MSAASamples     = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplSDL2_InitForVulkan( ( SDL_Window* ) info->window->handle );
	ImGui_ImplVulkan_Init( &init_info, render_pass->render_pass );

	begin_command_buffer( cmd );
	ImGui_ImplVulkan_CreateFontsTexture(
	    static_cast<VulkanCommandBuffer*>( cmd->handle )->command_buffer );
	end_command_buffer( cmd );
	immediate_submit( info->queue, cmd );

	ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void
vk_destroy_ui_context( const Device* idevice, UiContext* icontext )
{
	FT_ASSERT( icontext );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( context, icontext, VulkanUiContext );

	vkDestroyDescriptorPool( device->logical_device,
	                         context->desriptor_pool,
	                         nullptr );
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
	std::free( context );
}

void
vk_ui_begin_frame( UiContext*, CommandBuffer* icmd )
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
}

void
vk_ui_end_frame( UiContext*, CommandBuffer* icmd )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );

	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(),
	                                 cmd->command_buffer );

	ImGuiIO& io = ImGui::GetIO();
	if ( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

void*
vk_get_imgui_texture_id( const Image* iimage )
{
	FT_FROM_HANDLE( image, iimage, VulkanImage );
	return image->image_view;
}

void
vk_cmd_begin_render_pass( const CommandBuffer*       icmd,
                          const RenderPassBeginInfo* info )
{
	FT_ASSERT( info->render_pass );

	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );
	FT_FROM_HANDLE( render_pass, info->render_pass, VulkanRenderPass );

	static VkClearValue clear_values[ MAX_ATTACHMENTS_COUNT + 1 ];
	u32 clear_value_count = render_pass->interface.color_attachment_count;

	for ( u32 i = 0; i < render_pass->interface.color_attachment_count; ++i )
	{
		clear_values[ i ] = VkClearValue {};
		clear_values[ i ].color.float32[ 0 ] =
		    info->clear_values[ i ].color[ 0 ];
		clear_values[ i ].color.float32[ 1 ] =
		    info->clear_values[ i ].color[ 1 ];
		clear_values[ i ].color.float32[ 2 ] =
		    info->clear_values[ i ].color[ 2 ];
		clear_values[ i ].color.float32[ 3 ] =
		    info->clear_values[ i ].color[ 3 ];
	}

	if ( render_pass->interface.has_depth_stencil )
	{
		clear_value_count++;
		u32 idx             = render_pass->interface.color_attachment_count;
		clear_values[ idx ] = VkClearValue {};
		clear_values[ idx ].depthStencil.depth =
		    info->clear_values[ idx ].depth;
		clear_values[ idx ].depthStencil.stencil =
		    info->clear_values[ idx ].stencil;
	}

	VkRenderPassBeginInfo render_pass_begin_info {};
	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.pNext = nullptr;
	render_pass_begin_info.renderPass  = render_pass->render_pass;
	render_pass_begin_info.framebuffer = render_pass->framebuffer;
	render_pass_begin_info.renderArea.extent.width =
	    render_pass->interface.width;
	render_pass_begin_info.renderArea.extent.height =
	    render_pass->interface.height;
	render_pass_begin_info.renderArea.offset.x = 0;
	render_pass_begin_info.renderArea.offset.y = 0;
	render_pass_begin_info.clearValueCount     = clear_value_count;
	render_pass_begin_info.pClearValues        = clear_values;

	vkCmdBeginRenderPass( cmd->command_buffer,
	                      &render_pass_begin_info,
	                      VK_SUBPASS_CONTENTS_INLINE );
}

void
vk_cmd_end_render_pass( const CommandBuffer* icmd )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );
	vkCmdEndRenderPass( cmd->command_buffer );
}

void
vk_cmd_barrier( const CommandBuffer* icmd,
                u32,
                const MemoryBarrier*,
                u32                  buffer_barriers_count,
                const BufferBarrier* buffer_barriers,
                u32                  image_barriers_count,
                const ImageBarrier*  image_barriers )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );

	std::vector<VkBufferMemoryBarrier> buffer_memory_barriers(
	    buffer_barriers_count );
	std::vector<VkImageMemoryBarrier> image_memory_barriers(
	    image_barriers_count );

	// TODO: Queues
	VkAccessFlags src_access = VkAccessFlags( 0 );
	VkAccessFlags dst_access = VkAccessFlags( 0 );

	for ( u32 i = 0; i < buffer_barriers_count; ++i )
	{
		FT_ASSERT( buffer_barriers[ i ].buffer );
		FT_ASSERT( buffer_barriers[ i ].src_queue );
		FT_ASSERT( buffer_barriers[ i ].dst_queue );

		FT_FROM_HANDLE( buffer, buffer_barriers[ i ].buffer, VulkanBuffer );

		VkAccessFlags src_access_mask =
		    determine_access_flags( buffer_barriers[ i ].old_state );
		VkAccessFlags dst_access_mask =
		    determine_access_flags( buffer_barriers[ i ].new_state );

		VkBufferMemoryBarrier& barrier = buffer_memory_barriers[ i ];
		barrier.sType         = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.pNext         = nullptr;
		barrier.srcAccessMask = src_access_mask;
		barrier.dstAccessMask = dst_access_mask;
		barrier.srcQueueFamilyIndex =
		    buffer_barriers[ i ].src_queue->family_index;
		barrier.dstQueueFamilyIndex =
		    buffer_barriers[ i ].dst_queue->family_index;
		barrier.buffer = buffer->buffer;
		barrier.offset = buffer_barriers[ i ].offset;
		barrier.size   = buffer_barriers[ i ].size;

		src_access |= src_access_mask;
		dst_access |= dst_access_mask;
	}

	for ( u32 i = 0; i < image_barriers_count; ++i )
	{
		FT_ASSERT( image_barriers[ i ].image );
		FT_ASSERT( image_barriers[ i ].src_queue );
		FT_ASSERT( image_barriers[ i ].dst_queue );

		FT_FROM_HANDLE( image, image_barriers[ i ].image, VulkanImage );

		VkAccessFlags src_access_mask =
		    determine_access_flags( image_barriers[ i ].old_state );
		VkAccessFlags dst_access_mask =
		    determine_access_flags( image_barriers[ i ].new_state );

		VkImageMemoryBarrier& barrier = image_memory_barriers[ i ];

		barrier.sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.pNext         = nullptr;
		barrier.srcAccessMask = src_access_mask;
		barrier.dstAccessMask = dst_access_mask;
		barrier.oldLayout =
		    determine_image_layout( image_barriers[ i ].old_state );
		barrier.newLayout =
		    determine_image_layout( image_barriers[ i ].new_state );
		barrier.srcQueueFamilyIndex =
		    image_barriers[ i ].src_queue->family_index;
		barrier.dstQueueFamilyIndex =
		    image_barriers[ i ].dst_queue->family_index;
		barrier.image            = image->image;
		barrier.subresourceRange = get_image_subresource_range( image );

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
	                      nullptr,
	                      buffer_barriers_count,
	                      buffer_memory_barriers.data(),
	                      image_barriers_count,
	                      image_memory_barriers.data() );
};

void
vk_cmd_set_scissor( const CommandBuffer* icmd,
                    i32                  x,
                    i32                  y,
                    u32                  width,
                    u32                  height )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );

	VkRect2D scissor {};
	scissor.offset.x      = x;
	scissor.offset.y      = y;
	scissor.extent.width  = width;
	scissor.extent.height = height;
	vkCmdSetScissor( cmd->command_buffer, 0, 1, &scissor );
}

void
vk_cmd_set_viewport( const CommandBuffer* icmd,
                     f32                  x,
                     f32                  y,
                     f32                  width,
                     f32                  height,
                     f32                  min_depth,
                     f32                  max_depth )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );

	VkViewport viewport {};
	viewport.x        = x;
	viewport.y        = y + height;
	viewport.width    = width;
	viewport.height   = -height;
	viewport.minDepth = min_depth;
	viewport.maxDepth = max_depth;

	vkCmdSetViewport( cmd->command_buffer, 0, 1, &viewport );
}

void
vk_cmd_bind_pipeline( const CommandBuffer* icmd, const Pipeline* ipipeline )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );
	FT_FROM_HANDLE( pipeline, ipipeline, VulkanPipeline );

	vkCmdBindPipeline( cmd->command_buffer,
	                   to_vk_pipeline_bind_point( pipeline->interface.type ),
	                   pipeline->pipeline );
}

void
vk_cmd_draw( const CommandBuffer* icmd,
             u32                  vertex_count,
             u32                  instance_count,
             u32                  first_vertex,
             u32                  first_instance )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );

	vkCmdDraw( cmd->command_buffer,
	           vertex_count,
	           instance_count,
	           first_vertex,
	           first_instance );
}

void
vk_cmd_draw_indexed( const CommandBuffer* icmd,
                     u32                  index_count,
                     u32                  instance_count,
                     u32                  first_index,
                     i32                  vertex_offset,
                     u32                  first_instance )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );

	vkCmdDrawIndexed( cmd->command_buffer,
	                  index_count,
	                  instance_count,
	                  first_index,
	                  vertex_offset,
	                  first_instance );
}

void
vk_cmd_bind_vertex_buffer( const CommandBuffer* icmd,
                           const Buffer*        ibuffer,
                           const u64            offset )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );
	FT_FROM_HANDLE( buffer, ibuffer, VulkanBuffer );

	vkCmdBindVertexBuffers( cmd->command_buffer,
	                        0,
	                        1,
	                        &buffer->buffer,
	                        &offset );
}

void
vk_cmd_bind_index_buffer_u16( const CommandBuffer* icmd,
                              const Buffer*        ibuffer,
                              const u64            offset )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );
	FT_FROM_HANDLE( buffer, ibuffer, VulkanBuffer );

	vkCmdBindIndexBuffer( cmd->command_buffer,
	                      buffer->buffer,
	                      offset,
	                      VK_INDEX_TYPE_UINT16 );
}

void
vk_cmd_bind_index_buffer_u32( const CommandBuffer* icmd,
                              const Buffer*        ibuffer,
                              u64                  offset )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );
	FT_FROM_HANDLE( buffer, ibuffer, VulkanBuffer );

	vkCmdBindIndexBuffer( cmd->command_buffer,
	                      buffer->buffer,
	                      offset,
	                      VK_INDEX_TYPE_UINT32 );
}

void
vk_cmd_copy_buffer( const CommandBuffer* icmd,
                    const Buffer*        isrc,
                    u64                  src_offset,
                    Buffer*              idst,
                    u64                  dst_offset,
                    u64                  size )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );
	FT_FROM_HANDLE( src, isrc, VulkanBuffer );
	FT_FROM_HANDLE( dst, idst, VulkanBuffer );

	VkBufferCopy buffer_copy {};
	buffer_copy.srcOffset = src_offset;
	buffer_copy.dstOffset = dst_offset;
	buffer_copy.size      = size;

	vkCmdCopyBuffer( cmd->command_buffer,
	                 src->buffer,
	                 dst->buffer,
	                 1,
	                 &buffer_copy );
}

void
vk_cmd_copy_buffer_to_image( const CommandBuffer* icmd,
                             const Buffer*        isrc,
                             u64                  src_offset,
                             Image*               idst )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );
	FT_FROM_HANDLE( src, isrc, VulkanBuffer );
	FT_FROM_HANDLE( dst, idst, VulkanImage );

	auto dst_layers = get_image_subresource_layers( dst );

	VkBufferImageCopy buffer_to_image_copy_info {};
	buffer_to_image_copy_info.bufferOffset      = src_offset;
	buffer_to_image_copy_info.bufferImageHeight = 0;
	buffer_to_image_copy_info.bufferRowLength   = 0;
	buffer_to_image_copy_info.imageSubresource  = dst_layers;
	buffer_to_image_copy_info.imageOffset       = VkOffset3D { 0, 0, 0 };
	buffer_to_image_copy_info.imageExtent =
	    VkExtent3D { dst->interface.width, dst->interface.height, 1 };

	vkCmdCopyBufferToImage(
	    cmd->command_buffer,
	    src->buffer,
	    dst->image,
	    determine_image_layout( ResourceState::eTransferDst ),
	    1,
	    &buffer_to_image_copy_info );
}

void
vk_cmd_dispatch( const CommandBuffer* icmd,
                 u32                  group_count_x,
                 u32                  group_count_y,
                 u32                  group_count_z )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );

	vkCmdDispatch( cmd->command_buffer,
	               group_count_x,
	               group_count_y,
	               group_count_z );
}

void
vk_cmd_push_constants( const CommandBuffer* icmd,
                       const Pipeline*      ipipeline,
                       u64                  offset,
                       u64                  size,
                       const void*          data )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );
	FT_FROM_HANDLE( pipeline, ipipeline, VulkanPipeline );

	vkCmdPushConstants( cmd->command_buffer,
	                    pipeline->pipeline_layout,
	                    VK_SHADER_STAGE_VERTEX_BIT |
	                        VK_SHADER_STAGE_COMPUTE_BIT |
	                        VK_SHADER_STAGE_FRAGMENT_BIT,
	                    offset,
	                    size,
	                    data );
}

void
vk_cmd_blit_image( const CommandBuffer* icmd,
                   const Image*         isrc,
                   ResourceState        src_state,
                   Image*               idst,
                   ResourceState        dst_state,
                   Filter               filter )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );
	FT_FROM_HANDLE( src, isrc, VulkanImage );
	FT_FROM_HANDLE( dst, idst, VulkanImage );

	u32 barrier_count = 0;
	u32 index         = 0;

	ImageBarrier barriers[ 2 ] = {};
	barriers[ 0 ].src_queue    = cmd->interface.queue;
	barriers[ 0 ].dst_queue    = cmd->interface.queue;
	barriers[ 0 ].image        = &src->interface;
	barriers[ 0 ].old_state    = src_state;
	barriers[ 0 ].new_state    = ResourceState::eTransferSrc;

	barriers[ 1 ].src_queue = cmd->interface.queue;
	barriers[ 1 ].dst_queue = cmd->interface.queue;
	barriers[ 1 ].image     = &dst->interface;
	barriers[ 1 ].old_state = dst_state;
	barriers[ 1 ].new_state = ResourceState::eTransferDst;

	if ( src_state != ResourceState::eTransferSrc )
	{
		barrier_count++;
	}

	if ( dst_state != ResourceState::eTransferDst )
	{
		if ( barrier_count == 0 )
		{
			index = 1;
		}

		barrier_count++;
	}

	if ( barrier_count > 0 )
	{
		cmd_barrier( &cmd->interface,
		             0,
		             nullptr,
		             0,
		             nullptr,
		             barrier_count,
		             &barriers[ index ] );
	}

	auto src_layers = get_image_subresource_layers( src );
	auto dst_layers = get_image_subresource_layers( dst );

	VkImageBlit image_blit_info {};
	image_blit_info.srcOffsets[ 0 ] = VkOffset3D { 0, 0, 0 };
	image_blit_info.srcOffsets[ 1 ] =
	    VkOffset3D { static_cast<i32>( src->interface.width ),
	                 static_cast<i32>( src->interface.height ),
	                 1 };
	image_blit_info.dstOffsets[ 0 ] = VkOffset3D { 0, 0, 0 };
	image_blit_info.dstOffsets[ 1 ] =
	    VkOffset3D { ( i32 ) dst->interface.width,
	                 ( i32 ) dst->interface.height,
	                 1 };
	image_blit_info.srcSubresource = src_layers;
	image_blit_info.dstSubresource = dst_layers;

	vkCmdBlitImage( cmd->command_buffer,
	                src->image,
	                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	                dst->image,
	                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	                1,
	                &image_blit_info,
	                to_vk_filter( filter ) );
}

void
vk_cmd_clear_color_image( const CommandBuffer* icmd,
                          Image*               iimage,
                          Vector4              color )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );
	FT_FROM_HANDLE( image, iimage, VulkanImage );

	VkClearColorValue clear_color {};
	clear_color.float32[ 0 ] = color.r;
	clear_color.float32[ 1 ] = color.g;
	clear_color.float32[ 2 ] = color.b;
	clear_color.float32[ 3 ] = color.a;

	VkImageSubresourceRange range = get_image_subresource_range( image );

	vkCmdClearColorImage( cmd->command_buffer,
	                      image->image,
	                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	                      &clear_color,
	                      1,
	                      &range );
}

void
vk_cmd_draw_indexed_indirect( const CommandBuffer* icmd,
                              const Buffer*        ibuffer,
                              u64                  offset,
                              u32                  draw_count,
                              u32                  stride )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );
	FT_FROM_HANDLE( buffer, ibuffer, VulkanBuffer );

	vkCmdDrawIndexedIndirect( cmd->command_buffer,
	                          buffer->buffer,
	                          offset,
	                          draw_count,
	                          stride );
}

void
vk_cmd_bind_descriptor_set( const CommandBuffer* icmd,
                            u32                  first_set,
                            const DescriptorSet* iset,
                            const Pipeline*      ipipeline )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );
	FT_FROM_HANDLE( pipeline, ipipeline, VulkanPipeline );
	FT_FROM_HANDLE( set, iset, VulkanDescriptorSet );

	vkCmdBindDescriptorSets(
	    cmd->command_buffer,
	    to_vk_pipeline_bind_point( pipeline->interface.type ),
	    pipeline->pipeline_layout,
	    first_set,
	    1,
	    &set->descriptor_set,
	    0,
	    nullptr );
}

std::vector<char>
vk_read_shader( const std::string& shader_name )
{
	return fs::read_file_binary( fs::get_shaders_directory() + "vulkan/" +
	                             shader_name + ".bin" );
}

void
vk_create_renderer_backend( const RendererBackendInfo*, RendererBackend** p )
{
	FT_ASSERT( p );

	destroy_renderer_backend      = vk_destroy_renderer_backend;
	create_device                 = vk_create_device;
	destroy_device                = vk_destroy_device;
	create_queue                  = vk_create_queue;
	destroy_queue                 = vk_destroy_queue;
	queue_wait_idle               = vk_queue_wait_idle;
	queue_submit                  = vk_queue_submit;
	immediate_submit              = vk_immediate_submit;
	queue_present                 = vk_queue_present;
	create_semaphore              = vk_create_semaphore;
	destroy_semaphore             = vk_destroy_semaphore;
	create_fence                  = vk_create_fence;
	destroy_fence                 = vk_destroy_fence;
	wait_for_fences               = vk_wait_for_fences;
	reset_fences                  = vk_reset_fences;
	create_swapchain              = vk_create_swapchain;
	resize_swapchain              = vk_resize_swapchain;
	destroy_swapchain             = vk_destroy_swapchain;
	create_command_pool           = vk_create_command_pool;
	destroy_command_pool          = vk_destroy_command_pool;
	create_command_buffers        = vk_create_command_buffers;
	free_command_buffers          = vk_free_command_buffers;
	destroy_command_buffers       = vk_destroy_command_buffers;
	begin_command_buffer          = vk_begin_command_buffer;
	end_command_buffer            = vk_end_command_buffer;
	acquire_next_image            = vk_acquire_next_image;
	create_render_pass            = vk_create_render_pass;
	resize_render_pass            = vk_resize_render_pass;
	destroy_render_pass           = vk_destroy_render_pass;
	create_shader                 = vk_create_shader;
	destroy_shader                = vk_destroy_shader;
	create_descriptor_set_layout  = vk_create_descriptor_set_layout;
	destroy_descriptor_set_layout = vk_destroy_descriptor_set_layout;
	create_compute_pipeline       = vk_create_compute_pipeline;
	create_graphics_pipeline      = vk_create_graphics_pipeline;
	destroy_pipeline              = vk_destroy_pipeline;
	create_buffer                 = vk_create_buffer;
	destroy_buffer                = vk_destroy_buffer;
	map_memory                    = vk_map_memory;
	unmap_memory                  = vk_unmap_memory;
	create_sampler                = vk_create_sampler;
	destroy_sampler               = vk_destroy_sampler;
	create_image                  = vk_create_image;
	destroy_image                 = vk_destroy_image;
	create_descriptor_set         = vk_create_descriptor_set;
	destroy_descriptor_set        = vk_destroy_descriptor_set;
	update_descriptor_set         = vk_update_descriptor_set;
	create_ui_context             = vk_create_ui_context;
	destroy_ui_context            = vk_destroy_ui_context;
	ui_begin_frame                = vk_ui_begin_frame;
	ui_end_frame                  = vk_ui_end_frame;
	get_imgui_texture_id          = vk_get_imgui_texture_id;
	cmd_begin_render_pass         = vk_cmd_begin_render_pass;
	cmd_end_render_pass           = vk_cmd_end_render_pass;
	cmd_barrier                   = vk_cmd_barrier;
	cmd_set_scissor               = vk_cmd_set_scissor;
	cmd_set_viewport              = vk_cmd_set_viewport;
	cmd_bind_pipeline             = vk_cmd_bind_pipeline;
	cmd_draw                      = vk_cmd_draw;
	cmd_draw_indexed              = vk_cmd_draw_indexed;
	cmd_bind_vertex_buffer        = vk_cmd_bind_vertex_buffer;
	cmd_bind_index_buffer_u16     = vk_cmd_bind_index_buffer_u16;
	cmd_bind_index_buffer_u32     = vk_cmd_bind_index_buffer_u32;
	cmd_copy_buffer               = vk_cmd_copy_buffer;
	cmd_copy_buffer_to_image      = vk_cmd_copy_buffer_to_image;
	cmd_bind_descriptor_set       = vk_cmd_bind_descriptor_set;
	cmd_dispatch                  = vk_cmd_dispatch;
	cmd_push_constants            = vk_cmd_push_constants;
	cmd_blit_image                = vk_cmd_blit_image;
	cmd_clear_color_image         = vk_cmd_clear_color_image;
	cmd_draw_indexed_indirect     = vk_cmd_draw_indexed_indirect;

	read_shader = vk_read_shader;

	FT_INIT_INTERNAL( backend, *p, VulkanRendererBackend );

	// TODO: provide posibility to set allocator from user code
	backend->vulkan_allocator = nullptr;
	// TODO: same
	backend->api_version = VK_API_VERSION_1_2;

	volkInitialize();

	VkApplicationInfo app_info {};
	app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pNext              = nullptr;
	app_info.pApplicationName   = "Fluent";
	app_info.applicationVersion = VK_MAKE_VERSION( 0, 0, 1 );
	app_info.pEngineName        = "Fluent-Engine";
	app_info.engineVersion      = VK_MAKE_VERSION( 0, 0, 1 );
	app_info.apiVersion         = backend->api_version;

	u32  instance_create_flags;
	auto extensions = get_instance_extensions( instance_create_flags );
	auto layers     = get_instance_layers();

	VkInstanceCreateInfo instance_create_info {};
	instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_create_info.pNext = nullptr;
	instance_create_info.pApplicationInfo  = &app_info;
	instance_create_info.enabledLayerCount = static_cast<u32>( layers.size() );
	instance_create_info.ppEnabledLayerNames = layers.data();
	instance_create_info.enabledExtensionCount =
	    static_cast<u32>( extensions.size() );
	instance_create_info.ppEnabledExtensionNames = extensions.data();
	instance_create_info.flags                   = instance_create_flags;

	VK_ASSERT( vkCreateInstance( &instance_create_info,
	                             backend->vulkan_allocator,
	                             &backend->instance ) );

	volkLoadInstance( backend->instance );

#ifdef FLUENT_DEBUG
	create_debug_messenger( backend );
#endif

	// pick physical device
	backend->physical_device = VK_NULL_HANDLE;
	u32 device_count         = 0;
	vkEnumeratePhysicalDevices( backend->instance, &device_count, nullptr );
	FT_ASSERT( device_count != 0 );
	std::vector<VkPhysicalDevice> physical_devices( device_count );
	vkEnumeratePhysicalDevices( backend->instance,
	                            &device_count,
	                            physical_devices.data() );
	backend->physical_device = physical_devices[ 0 ];

	// select best physical device
	for ( u32 i = 0; i < device_count; ++i )
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

} // namespace fluent
#endif
