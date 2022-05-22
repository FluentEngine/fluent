#ifdef VULKAN_BACKEND

#include <alloca.h>
#include <tiny_image_format/tinyimageformat_apis.h>
#include <hashmap.c/hashmap.h>
#include "log/log.h"
#include "wsi/wsi.h"
#include "renderer/renderer_backend_functions.h"
#include "vulkan_backend.h"

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

static inline u32
clamp_u32( u32 d, u32 min, u32 max )
{
	return d < min ? min : ( max < d ) ? max : d;
}

struct RenderPassMapItem
{
	struct RenderPassBeginInfo info;
	VkRenderPass               value;
};

struct FramebufferMapItem
{
	struct RenderPassBeginInfo info;
	VkFramebuffer              value;
};

static b32
compare_pass_info( const void* a, const void* b, void* udata )
{
	const struct RenderPassBeginInfo* rpa = a;
	const struct RenderPassBeginInfo* rpb = b;
	if ( rpa->color_attachment_count != rpb->color_attachment_count )
		return 0;

	for ( u32 i = 0; i < rpa->color_attachment_count; ++i )
	{
		if ( rpa->color_attachments[ i ]->format !=
		     rpb->color_attachments[ i ]->format )
		{
			return 1;
		}

		if ( rpa->color_attachments[ i ]->sample_count !=
		     rpb->color_attachments[ i ]->sample_count )
		{
			return 1;
		}

		if ( rpa->color_attachment_load_ops[ i ] !=
		     rpb->color_attachment_load_ops[ i ] )
		{
			return 1;
		}

		if ( rpa->color_image_states[ i ] != rpb->color_image_states[ i ] )
		{
			return 1;
		}
	}

	if ( ( rpa->depth_stencil != rpb->depth_stencil ) &&
	     ( ( rpa->depth_stencil == NULL ) || ( rpb->depth_stencil == NULL ) ) )
	{
		return 1;
	}

	if ( rpa->depth_stencil )
	{
		if ( rpa->depth_stencil != rpb->depth_stencil )
		{
			return 1;
		}

		if ( rpa->depth_stencil->sample_count !=
		     rpb->depth_stencil->sample_count )
		{
			return 1;
		}

		if ( rpa->depth_stencil_load_op != rpb->depth_stencil_load_op )
		{
			return 1;
		}

		if ( rpa->depth_stencil_state != rpb->depth_stencil_state )
		{
			return 1;
		}
	}

	return 0;
}

static u64
hash_pass_info( const void* item, u64 seed0, u64 seed1 )
{
	// TODO: hash function
	return 0;
}

static b32
compare_framebuffer_info( const void* a, const void* b, void* udata )
{
	const struct RenderPassBeginInfo* rpa = a;
	const struct RenderPassBeginInfo* rpb = b;

	if ( rpa->color_attachment_count != rpb->color_attachment_count )
	{
		return 1;
	}

	for ( u32 i = 0; i < rpa->color_attachment_count; ++i )
	{
		if ( rpa->color_attachments[ i ] != rpb->color_attachments[ i ] )
		{
			return 1;
		}
	}

	if ( ( rpa->depth_stencil != rpb->depth_stencil ) &&
	     ( ( rpa->depth_stencil == NULL ) || ( rpb->depth_stencil == NULL ) ) )
	{
		return 1;
	}

	if ( rpa->depth_stencil )
	{
		if ( rpa->depth_stencil != rpb->depth_stencil )
		{
			return 1;
		}
	}

	return 0;
}

static u64
hash_framebuffer_info( const void* item, u64 seed0, u64 seed1 )
{
	// TODO: hash function
	return 0;
}

static struct VulkanDevice* devices[ MAX_DEVICE_COUNT ];
static struct hashmap*      passes[ MAX_DEVICE_COUNT ]       = { NULL };
static struct hashmap*      framebuffers[ MAX_DEVICE_COUNT ] = { NULL };

static inline VkFormat
to_vk_format( enum Format format )
{
	return ( VkFormat ) TinyImageFormat_ToVkFormat(
	    ( TinyImageFormat ) format );
}

static inline enum Format
from_vk_format( VkFormat format )
{
	return ( enum Format ) TinyImageFormat_FromVkFormat(
	    ( TinyImageFormat_VkFormat ) format );
}

static inline VkSampleCountFlagBits
to_vk_sample_count( u32 sample_count )
{
	// TODO: assert?
	return ( VkSampleCountFlagBits ) sample_count;
}

static inline VkAttachmentLoadOp
to_vk_load_op( enum AttachmentLoadOp load_op )
{
	switch ( load_op )
	{
	case FT_ATTACHMENT_LOAD_OP_CLEAR: return VK_ATTACHMENT_LOAD_OP_CLEAR;
	case FT_ATTACHMENT_LOAD_OP_DONT_CARE:
		return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	case FT_ATTACHMENT_LOAD_OP_LOAD: return VK_ATTACHMENT_LOAD_OP_LOAD;
	default: FT_ASSERT( 0 ); return ( VkAttachmentLoadOp ) -1;
	}
}

static inline VkQueueFlagBits
to_vk_queue_type( enum QueueType type )
{
	switch ( type )
	{
	case FT_QUEUE_TYPE_GRAPHICS: return VK_QUEUE_GRAPHICS_BIT;
	case FT_QUEUE_TYPE_COMPUTE: return VK_QUEUE_COMPUTE_BIT;
	case FT_QUEUE_TYPE_TRANSFER: return VK_QUEUE_TRANSFER_BIT;
	default: FT_ASSERT( 0 ); return ( VkQueueFlagBits ) -1;
	}
}

static inline VkVertexInputRate
to_vk_vertex_input_rate( enum VertexInputRate input_rate )
{
	switch ( input_rate )
	{
	case FT_VERTEX_INPUT_RATE_VERTEX: return VK_VERTEX_INPUT_RATE_VERTEX;
	case FT_VERTEX_INPUT_RATE_INSTANCE: return VK_VERTEX_INPUT_RATE_INSTANCE;
	default: FT_ASSERT( 0 ); return ( VkVertexInputRate ) -1;
	}
}

static inline VkCullModeFlagBits
to_vk_cull_mode( enum CullMode cull_mode )
{
	switch ( cull_mode )
	{
	case FT_CULL_MODE_BACK: return VK_CULL_MODE_BACK_BIT;
	case FT_CULL_MODE_FRONT: return VK_CULL_MODE_FRONT_BIT;
	case FT_CULL_MODE_NONE: return VK_CULL_MODE_NONE;
	default: FT_ASSERT( 0 ); return ( VkCullModeFlagBits ) -1;
	}
}

static inline VkFrontFace
to_vk_front_face( enum FrontFace front_face )
{
	switch ( front_face )
	{
	case FT_FRONT_FACE_CLOCKWISE: return VK_FRONT_FACE_CLOCKWISE;
	case FT_FRONT_FACE_COUNTER_CLOCKWISE:
		return VK_FRONT_FACE_COUNTER_CLOCKWISE;
	default: FT_ASSERT( 0 ); return ( VkFrontFace ) -1;
	}
}

static inline VkCompareOp
to_vk_compare_op( enum CompareOp op )
{
	switch ( op )
	{
	case FT_COMPARE_OP_NEVER: return VK_COMPARE_OP_NEVER;
	case FT_COMPARE_OP_LESS: return VK_COMPARE_OP_LESS;
	case FT_COMPARE_OP_EQUAL: return VK_COMPARE_OP_EQUAL;
	case FT_COMPARE_OP_LESS_OR_EQUAL: return VK_COMPARE_OP_LESS_OR_EQUAL;
	case FT_COMPARE_OP_GREATER: return VK_COMPARE_OP_GREATER;
	case FT_COMPARE_OP_NOT_EQUAL: return VK_COMPARE_OP_NOT_EQUAL;
	case FT_COMPARE_OP_GREATER_OR_EQUAL: return VK_COMPARE_OP_GREATER_OR_EQUAL;
	case FT_COMPARE_OP_ALWAYS: return VK_COMPARE_OP_ALWAYS;
	default: FT_ASSERT( 0 ); return ( VkCompareOp ) -1;
	}
}

static inline VkShaderStageFlagBits
to_vk_shader_stage( enum ShaderStage shader_stage )
{
	switch ( shader_stage )
	{
	case FT_SHADER_STAGE_VERTEX: return VK_SHADER_STAGE_VERTEX_BIT;
	case FT_SHADER_STAGE_TESSELLATION_CONTROL:
		return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	case FT_SHADER_STAGE_TESSELLATION_EVALUATION:
		return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	case FT_SHADER_STAGE_GEOMETRY: return VK_SHADER_STAGE_GEOMETRY_BIT;
	case FT_SHADER_STAGE_FRAGMENT: return VK_SHADER_STAGE_FRAGMENT_BIT;
	case FT_SHADER_STAGE_COMPUTE: return VK_SHADER_STAGE_COMPUTE_BIT;
	default: FT_ASSERT( 0 ); return ( VkShaderStageFlagBits ) -1;
	}
}

static inline VkPipelineBindPoint
to_vk_pipeline_bind_point( enum PipelineType type )
{
	switch ( type )
	{
	case FT_PIPELINE_TYPE_COMPUTE: return VK_PIPELINE_BIND_POINT_COMPUTE;
	case FT_PIPELINE_TYPE_GRAPHICS: return VK_PIPELINE_BIND_POINT_GRAPHICS;
	default: FT_ASSERT( 0 ); return ( VkPipelineBindPoint ) -1;
	}
}

static inline VkDescriptorType
to_vk_descriptor_type( enum DescriptorType descriptor_type )
{
	switch ( descriptor_type )
	{
	case FT_DESCRIPTOR_TYPE_SAMPLER: return VK_DESCRIPTOR_TYPE_SAMPLER;
	case FT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	case FT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
		return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	case FT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
		return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
	case FT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
		return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
	case FT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	case FT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	case FT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	case FT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
		return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
	case FT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
		return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	default: FT_ASSERT( 0 ); return ( VkDescriptorType ) -1;
	}
}

static inline VkFilter
to_vk_filter( enum Filter filter )
{
	switch ( filter )
	{
	case FT_FILTER_LINEAR: return VK_FILTER_LINEAR;
	case FT_FILTER_NEAREST: return VK_FILTER_NEAREST;
	default: FT_ASSERT( 0 ); return ( VkFilter ) -1;
	}
}

static inline VkSamplerMipmapMode
to_vk_sampler_mipmap_mode( enum SamplerMipmapMode mode )
{
	switch ( mode )
	{
	case FT_SAMPLER_MIPMAP_MODE_NEAREST: return VK_SAMPLER_MIPMAP_MODE_NEAREST;
	case FT_SAMPLER_MIPMAP_MODE_LINEAR: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	default: FT_ASSERT( 0 ); return ( VkSamplerMipmapMode ) -1;
	}
}

static inline VkSamplerAddressMode
to_vk_sampler_address_mode( enum SamplerAddressMode mode )
{
	switch ( mode )
	{
	case FT_SAMPLER_ADDRESS_MODE_REPEAT: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case FT_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:
		return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	case FT_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case FT_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	default: FT_ASSERT( 0 ); return ( VkSamplerAddressMode ) -1;
	}
}

static inline VkPolygonMode
to_vk_polygon_mode( enum PolygonMode mode )
{
	switch ( mode )
	{
	case FT_POLYGON_MODE_FILL: return VK_POLYGON_MODE_FILL;
	case FT_POLYGON_MODE_LINE: return VK_POLYGON_MODE_LINE;
	default: FT_ASSERT( 0 ); return ( VkPolygonMode ) -1;
	}
}

static inline VkPrimitiveTopology
to_vk_primitive_topology( enum PrimitiveTopology topology )
{
	switch ( topology )
	{
	case FT_PRIMITIVE_TOPOLOGY_LINE_LIST:
		return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	case FT_PRIMITIVE_TOPOLOGY_LINE_STRIP:
		return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
	case FT_PRIMITIVE_TOPOLOGY_POINT_LIST:
		return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	case FT_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	case FT_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	default: FT_ASSERT( 0 ); return ( VkPrimitiveTopology ) -1;
	}
}

static inline VkAccessFlags
determine_access_flags( enum ResourceState resource_state )
{
	VkAccessFlags access_flags = 0;

	if ( resource_state & FT_RESOURCE_STATE_GENERAL )
		access_flags |=
		    ( VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT );

	if ( resource_state & FT_RESOURCE_STATE_COLOR_ATTACHMENT )
		access_flags |= ( VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
		                  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT );

	if ( resource_state & FT_RESOURCE_STATE_DEPTH_STENCIL_WRITE )
		access_flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	if ( resource_state & FT_RESOURCE_STATE_DEPTH_STENCIL_READ_ONLY )
		access_flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

	if ( resource_state & FT_RESOURCE_STATE_SHADER_READ_ONLY )
		access_flags |= VK_ACCESS_SHADER_READ_BIT;

	if ( resource_state & FT_RESOURCE_STATE_TRANSFER_SRC )
		access_flags |= VK_ACCESS_TRANSFER_READ_BIT;

	if ( resource_state & FT_RESOURCE_STATE_TRANSFER_DST )
		access_flags |= VK_ACCESS_TRANSFER_WRITE_BIT;

	if ( resource_state & FT_RESOURCE_STATE_PRESENT )
		access_flags |= VK_ACCESS_MEMORY_READ_BIT;

	return access_flags;
}

static inline VkPipelineStageFlags
determine_pipeline_stage_flags( VkAccessFlags  access_flags,
                                enum QueueType queue_type )
{
	// TODO: FIX THIS
	VkPipelineStageFlags flags = 0;

	switch ( queue_type )
	{
	case FT_QUEUE_TYPE_GRAPHICS:
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
	case FT_QUEUE_TYPE_COMPUTE:
	{
		break;
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
determine_image_layout( enum ResourceState resource_state )
{
	if ( resource_state & FT_RESOURCE_STATE_GENERAL )
		return VK_IMAGE_LAYOUT_GENERAL;

	if ( resource_state & FT_RESOURCE_STATE_COLOR_ATTACHMENT )
		return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	if ( resource_state & FT_RESOURCE_STATE_DEPTH_STENCIL_WRITE )
		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	if ( resource_state & FT_RESOURCE_STATE_DEPTH_STENCIL_READ_ONLY )
		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

	if ( resource_state & FT_RESOURCE_STATE_SHADER_READ_ONLY )
		return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	if ( resource_state & FT_RESOURCE_STATE_PRESENT )
		return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	if ( resource_state & FT_RESOURCE_STATE_TRANSFER_SRC )
		return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

	if ( resource_state & FT_RESOURCE_STATE_TRANSFER_DST )
		return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

	return VK_IMAGE_LAYOUT_UNDEFINED;
}

static inline VmaMemoryUsage
determine_vma_memory_usage( enum MemoryUsage usage )
{
	switch ( usage )
	{
	case FT_MEMORY_USAGE_CPU_ONLY: return VMA_MEMORY_USAGE_CPU_ONLY;
	case FT_MEMORY_USAGE_GPU_ONLY: return VMA_MEMORY_USAGE_GPU_ONLY;
	case FT_MEMORY_USAGE_CPU_COPY: return VMA_MEMORY_USAGE_CPU_COPY;
	case FT_MEMORY_USAGE_CPU_TO_GPU: return VMA_MEMORY_USAGE_CPU_TO_GPU;
	case FT_MEMORY_USAGE_GPU_TO_CPU: return VMA_MEMORY_USAGE_GPU_TO_CPU;
	default: break;
	}
	FT_ASSERT( 0 );
	return ( VmaMemoryUsage ) -1;
}

static inline VkBufferUsageFlags
determine_vk_buffer_usage( enum DescriptorType descriptor_type,
                           enum MemoryUsage    memory_usage )
{
	// TODO: determine buffer usage flags
	VkBufferUsageFlags buffer_usage = ( VkBufferUsageFlags ) 0;

	if ( memory_usage == FT_MEMORY_USAGE_CPU_TO_GPU ||
	     memory_usage == FT_MEMORY_USAGE_CPU_ONLY )
	{
		buffer_usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	}

	if ( descriptor_type & FT_DESCRIPTOR_TYPE_VERTEX_BUFFER )
	{
		buffer_usage |= ( VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
		                  VK_BUFFER_USAGE_TRANSFER_DST_BIT );
	}

	if ( descriptor_type & FT_DESCRIPTOR_TYPE_INDEX_BUFFER )
	{
		buffer_usage |= ( VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
		                  VK_BUFFER_USAGE_TRANSFER_DST_BIT );
	}

	if ( descriptor_type & FT_DESCRIPTOR_TYPE_UNIFORM_BUFFER )
	{
		buffer_usage |= ( VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
		                  VK_BUFFER_USAGE_TRANSFER_DST_BIT );
	}

	if ( descriptor_type & FT_DESCRIPTOR_TYPE_INDIRECT_BUFFER )
	{
		buffer_usage |= ( VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
		                  VK_BUFFER_USAGE_TRANSFER_DST_BIT );
	}

	if ( descriptor_type & FT_DESCRIPTOR_TYPE_STORAGE_BUFFER )
	{
		buffer_usage |= ( VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		                  VK_BUFFER_USAGE_TRANSFER_DST_BIT );
	}

	return buffer_usage;
}

static inline VkImageUsageFlags
determine_vk_image_usage( enum DescriptorType descriptor_type )
{
	VkImageUsageFlags image_usage = ( VkImageUsageFlags ) 0;

	if ( descriptor_type & FT_DESCRIPTOR_TYPE_SAMPLED_IMAGE )
	{
		image_usage |= VK_IMAGE_USAGE_SAMPLED_BIT |
		               VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
		               VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	if ( descriptor_type & FT_DESCRIPTOR_TYPE_STORAGE_IMAGE )
	{
		image_usage |=
		    VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	if ( descriptor_type & FT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT )
	{
		image_usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
	}

	if ( descriptor_type & FT_DESCRIPTOR_TYPE_COLOR_ATTACHMENT )
	{
		image_usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}

	if ( descriptor_type & FT_DESCRIPTOR_TYPE_DEPTH_STENCIL_ATTACHMENT )
	{
		image_usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}

	if ( descriptor_type & FT_DESCRIPTOR_TYPE_TRANSIENT_ATTACHMENT )
	{
		image_usage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
	}

	return image_usage;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
vulkan_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT             flags,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void*                                       user_data )
{
	( void ) flags;
	( void ) user_data;

	static char const* prefix = "[Vulkan]:";

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

static inline void
create_debug_messenger( struct VulkanRendererBackend* backend )
{
	VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info = {};
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

static inline void
get_instance_extensions( const struct RendererBackendInfo* info,
                         u32*         instance_create_flags,
                         u32*         count,
                         const char** names )
{
	*instance_create_flags = 0;

	struct WsiInfo* wsi = info->wsi_info;

	if ( names == NULL )
	{
		*count = wsi->vulkan_instance_extension_count;
	}

	u32 e = 0;
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

	u32 extension_count = 0;
	vkEnumerateInstanceExtensionProperties( NULL, &extension_count, NULL );
	ALLOC_STACK_ARRAY( VkExtensionProperties,
	                   extension_properties,
	                   extension_count );
	vkEnumerateInstanceExtensionProperties( NULL,
	                                        &extension_count,
	                                        extension_properties );

	for ( u32 i = 0; i < extension_count; ++i )
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

static inline void
get_instance_layers( u32* count, const char** names )
{
	*count = 0;
#ifdef FLUENT_DEBUG
	( *count )++;
	if ( names )
	{
		names[ 0 ] = "VK_LAYER_KHRONOS_validation";
	}
#endif
}

static inline u32
find_queue_family_index( VkPhysicalDevice physical_device,
                         enum QueueType   queue_type )
{
	u32 index = UINT32_MAX;

	u32 queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties( physical_device,
	                                          &queue_family_count,
	                                          NULL );
	ALLOC_STACK_ARRAY( VkQueueFamilyProperties,
	                   queue_families,
	                   queue_family_count );
	vkGetPhysicalDeviceQueueFamilyProperties( physical_device,
	                                          &queue_family_count,
	                                          queue_families );

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
	FT_ASSERT( index != UINT32_MAX );

	return index;
}

static inline VkImageAspectFlags
get_aspect_mask( enum Format format )
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
get_image_subresource_range( const struct VulkanImage* image )
{
	VkImageSubresourceRange image_subresource_range = {};
	image_subresource_range.aspectMask =
	    get_aspect_mask( image->interface.format );
	image_subresource_range.baseMipLevel   = 0;
	image_subresource_range.levelCount     = image->interface.mip_level_count;
	image_subresource_range.baseArrayLayer = 0;
	image_subresource_range.layerCount     = image->interface.layer_count;

	return image_subresource_range;
}

static inline VkImageSubresourceLayers
get_image_subresource_layers( const struct VulkanImage* image )
{
	VkImageSubresourceRange subresourceRange =
	    get_image_subresource_range( image );
	VkImageSubresourceLayers layers = { subresourceRange.aspectMask,
		                                subresourceRange.baseMipLevel,
		                                subresourceRange.baseArrayLayer,
		                                subresourceRange.layerCount };
	return layers;
}

static void
vk_destroy_renderer_backend( struct RendererBackend* ibackend )
{
	FT_FROM_HANDLE( backend, ibackend, VulkanRendererBackend );

#ifdef FLUENT_DEBUG
	vkDestroyDebugUtilsMessengerEXT( backend->instance,
	                                 backend->debug_messenger,
	                                 backend->vulkan_allocator );
#endif
	vkDestroyInstance( backend->instance, backend->vulkan_allocator );
	free( backend );
}

static void*
vk_map_memory( const struct Device* idevice, struct Buffer* ibuffer )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( buffer, ibuffer, VulkanBuffer );

	vmaMapMemory( device->memory_allocator,
	              buffer->allocation,
	              &buffer->interface.mapped_memory );

	return buffer->interface.mapped_memory;
}

static void
vk_unmap_memory( const struct Device* idevice, struct Buffer* ibuffer )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( buffer, ibuffer, VulkanBuffer );

	vmaUnmapMemory( device->memory_allocator, buffer->allocation );
	buffer->interface.mapped_memory = NULL;
}

static void
vk_create_device( const struct RendererBackend* ibackend,
                  const struct DeviceInfo*      info,
                  struct Device**               p )
{
	FT_FROM_HANDLE( backend, ibackend, VulkanRendererBackend );

	FT_INIT_INTERNAL( device, *p, VulkanDevice );

	b32 found_device_slot = 0;
	for ( u32 i = 0; i < MAX_DEVICE_COUNT; ++i )
	{
		if ( devices[ i ] == NULL )
		{
			device->index     = i;
			found_device_slot = 1;
			devices[ i ]      = device;
			break;
		}
	}

	FT_ASSERT( found_device_slot &&
	           "MAX_DEVICE_COUNT less than created device count" );

	device->interface.api    = backend->interface.api;
	device->vulkan_allocator = backend->vulkan_allocator;
	device->instance         = backend->instance;
	device->physical_device  = backend->physical_device;

	u32 queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties( device->physical_device,
	                                          &queue_family_count,
	                                          NULL );
	ALLOC_STACK_ARRAY( VkQueueFamilyProperties,
	                   queue_families,
	                   queue_family_count );
	vkGetPhysicalDeviceQueueFamilyProperties( device->physical_device,
	                                          &queue_family_count,
	                                          queue_families );

	u32                     queue_create_info_count = 0;
	VkDeviceQueueCreateInfo queue_create_infos[ FT_QUEUE_TYPE_COUNT ];
	f32                     queue_priority = 1.0f;

	// TODO: Select queues
	for ( u32 i = 0; i < queue_family_count; ++i )
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

	u32 device_extension_count = 0;
	vkEnumerateDeviceExtensionProperties( device->physical_device,
	                                      NULL,
	                                      &device_extension_count,
	                                      NULL );
	ALLOC_STACK_ARRAY( VkExtensionProperties,
	                   supported_device_extensions,
	                   device_extension_count + 2 ); // TODO:
	vkEnumerateDeviceExtensionProperties( device->physical_device,
	                                      NULL,
	                                      &device_extension_count,
	                                      supported_device_extensions );

	b32 portability_subset = 0;
	for ( u32 i = 0; i < device_extension_count; ++i )
	{
		if ( !strcmp( supported_device_extensions[ i ].extensionName,
		              "VK_KHR_portability_subset" ) )
		{
			portability_subset = 1;
		}
	}

	device_extension_count = 2 + portability_subset;
	ALLOC_STACK_ARRAY( const char*, device_extensions, device_extension_count );
	device_extensions[ 0 ] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
	device_extensions[ 1 ] = VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME;
	if ( portability_subset )
	{
		device_extensions[ 2 ] = "VK_KHR_portability_subset";
	}

	// TODO: check support
	VkPhysicalDeviceFeatures used_features = {};
	used_features.fillModeNonSolid         = VK_TRUE;
	used_features.multiDrawIndirect        = VK_TRUE;
	used_features.sampleRateShading        = VK_TRUE;
	used_features.samplerAnisotropy        = VK_TRUE;

	VkPhysicalDeviceDescriptorIndexingFeatures
	    descriptor_indexing_features = {};
	descriptor_indexing_features.sType =
	    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
	descriptor_indexing_features.descriptorBindingPartiallyBound = 1;

	VkPhysicalDeviceMultiviewFeatures multiview_features = {};
	multiview_features.sType =
	    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES;
	multiview_features.multiview = 1;
	multiview_features.pNext     = &descriptor_indexing_features;

	VkPhysicalDeviceShaderDrawParametersFeatures
	    shader_draw_parameters_features = {};
	shader_draw_parameters_features.sType =
	    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETER_FEATURES;
	shader_draw_parameters_features.shaderDrawParameters = 1;
	shader_draw_parameters_features.pNext                = &multiview_features;

	VkDeviceCreateInfo device_create_info = {};
	device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.pNext = &shader_draw_parameters_features;
	device_create_info.flags = 0;
	device_create_info.queueCreateInfoCount    = queue_create_info_count;
	device_create_info.pQueueCreateInfos       = queue_create_infos;
	device_create_info.enabledLayerCount       = 0;
	device_create_info.ppEnabledLayerNames     = NULL;
	device_create_info.enabledExtensionCount   = device_extension_count;
	device_create_info.ppEnabledExtensionNames = device_extensions;
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

	VmaAllocatorCreateInfo vma_allocator_create_info = {};
	vma_allocator_create_info.instance               = device->instance;
	vma_allocator_create_info.physicalDevice         = device->physical_device;
	vma_allocator_create_info.device                 = device->logical_device;
	vma_allocator_create_info.flags                  = 0;
	vma_allocator_create_info.pAllocationCallbacks   = device->vulkan_allocator;
	vma_allocator_create_info.pVulkanFunctions       = &vulkanFunctions;
	vma_allocator_create_info.vulkanApiVersion       = backend->api_version;

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

	VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
	descriptor_pool_create_info.sType =
	    VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptor_pool_create_info.pNext = NULL;
	descriptor_pool_create_info.flags =
	    VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	descriptor_pool_create_info.maxSets       = 2048 * POOL_SIZE_COUNT;
	descriptor_pool_create_info.poolSizeCount = POOL_SIZE_COUNT;
	descriptor_pool_create_info.pPoolSizes    = pool_sizes;

	VK_ASSERT( vkCreateDescriptorPool( device->logical_device,
	                                   &descriptor_pool_create_info,
	                                   device->vulkan_allocator,
	                                   &device->descriptor_pool ) );

	passes[ device->index ] = hashmap_new( sizeof( struct RenderPassMapItem ),
	                                       0,
	                                       0,
	                                       0,
	                                       hash_pass_info,
	                                       compare_pass_info,
	                                       NULL,
	                                       NULL );

	framebuffers[ device->index ] =
	    hashmap_new( sizeof( struct FramebufferMapItem ),
	                 0,
	                 0,
	                 0,
	                 hash_framebuffer_info,
	                 compare_framebuffer_info,
	                 NULL,
	                 NULL );
}

static void
vk_destroy_device( struct Device* idevice )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	u64   iter = 0;
	void* item;

	while ( hashmap_iter( framebuffers[ device->index ], &iter, &item ) )
	{
		struct FramebufferMapItem* fb = item;
		vkDestroyFramebuffer( device->logical_device,
		                      fb->value,
		                      device->vulkan_allocator );
	}
	hashmap_free( framebuffers[ device->index ] );

	iter = 0;

	while ( hashmap_iter( passes[ device->index ], &iter, &item ) )
	{
		struct RenderPassMapItem* pass = item;
		vkDestroyRenderPass( device->logical_device,
		                     pass->value,
		                     device->vulkan_allocator );
	}

	hashmap_free( passes[ device->index ] );

	vkDestroyDescriptorPool( device->logical_device,
	                         device->descriptor_pool,
	                         device->vulkan_allocator );
	vmaDestroyAllocator( device->memory_allocator );
	vkDestroyDevice( device->logical_device, device->vulkan_allocator );

	devices[ device->index ] = NULL;

	free( device );
}

static void
vk_create_queue( const struct Device*    idevice,
                 const struct QueueInfo* info,
                 struct Queue**          p )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	FT_INIT_INTERNAL( queue, *p, VulkanQueue );

	u32 index =
	    find_queue_family_index( device->physical_device, info->queue_type );

	queue->interface.family_index = index;
	queue->interface.type         = info->queue_type;
	vkGetDeviceQueue( device->logical_device, index, 0, &queue->queue );
}

static void
vk_destroy_queue( struct Queue* iqueue )
{
	FT_FROM_HANDLE( queue, iqueue, VulkanQueue );
	free( queue );
}

static void
vk_queue_wait_idle( const struct Queue* iqueue )
{
	FT_FROM_HANDLE( queue, iqueue, VulkanQueue );
	vkQueueWaitIdle( queue->queue );
}

static void
vk_queue_submit( const struct Queue*           iqueue,
                 const struct QueueSubmitInfo* info )
{
	FT_FROM_HANDLE( queue, iqueue, VulkanQueue );

	VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;

	ALLOC_STACK_ARRAY( VkSemaphore,
	                   wait_semaphores,
	                   info->wait_semaphore_count );
	ALLOC_STACK_ARRAY( VkCommandBuffer,
	                   command_buffers,
	                   info->command_buffer_count );
	ALLOC_STACK_ARRAY( VkSemaphore,
	                   signal_semaphores,
	                   info->signal_semaphore_count );

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
	submit_info.pNext                = NULL;
	submit_info.waitSemaphoreCount   = info->wait_semaphore_count;
	submit_info.pWaitSemaphores      = wait_semaphores;
	submit_info.pWaitDstStageMask    = &wait_dst_stage_mask;
	submit_info.commandBufferCount   = info->command_buffer_count;
	submit_info.pCommandBuffers      = command_buffers;
	submit_info.signalSemaphoreCount = info->signal_semaphore_count;
	submit_info.pSignalSemaphores    = signal_semaphores;

	vkQueueSubmit(
	    queue->queue,
	    1,
	    &submit_info,
	    info->signal_fence
	        ? ( ( struct VulkanFence* ) ( info->signal_fence->handle ) )->fence
	        : VK_NULL_HANDLE );
}

static void
vk_immediate_submit( const struct Queue* iqueue, struct CommandBuffer* cmd )
{
	struct QueueSubmitInfo queue_submit_info = {};
	queue_submit_info.command_buffer_count   = 1;
	queue_submit_info.command_buffers        = &cmd;
	queue_submit( iqueue, &queue_submit_info );
	queue_wait_idle( iqueue );
}

static void
vk_queue_present( const struct Queue*            iqueue,
                  const struct QueuePresentInfo* info )
{
	FT_FROM_HANDLE( swapchain, info->swapchain, VulkanSwapchain );
	FT_FROM_HANDLE( queue, iqueue, VulkanQueue );

	ALLOC_STACK_ARRAY( VkSemaphore,
	                   wait_semaphores,
	                   info->wait_semaphore_count );

	for ( u32 i = 0; i < info->wait_semaphore_count; ++i )
	{
		FT_FROM_HANDLE( semaphore,
		                info->wait_semaphores[ i ],
		                VulkanSemaphore );
		wait_semaphores[ i ] = semaphore->semaphore;
	}

	VkPresentInfoKHR present_info = {};

	present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pNext              = NULL;
	present_info.waitSemaphoreCount = info->wait_semaphore_count;
	present_info.pWaitSemaphores    = wait_semaphores;
	present_info.swapchainCount     = 1;
	present_info.pSwapchains        = &swapchain->swapchain;
	present_info.pImageIndices      = &info->image_index;
	present_info.pResults           = NULL;

	vkQueuePresentKHR( queue->queue, &present_info );
}

static void
vk_create_semaphore( const struct Device* idevice, struct Semaphore** p )
{
	FT_ASSERT( p );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	FT_INIT_INTERNAL( semaphore, *p, VulkanSemaphore );

	VkSemaphoreCreateInfo semaphore_create_info = {};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphore_create_info.pNext = NULL;
	semaphore_create_info.flags = 0;

	VK_ASSERT( vkCreateSemaphore( device->logical_device,
	                              &semaphore_create_info,
	                              device->vulkan_allocator,
	                              &semaphore->semaphore ) );
}

static void
vk_destroy_semaphore( const struct Device* idevice,
                      struct Semaphore*    isemaphore )
{
	FT_ASSERT( isemaphore );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( semaphore, isemaphore, VulkanSemaphore );

	vkDestroySemaphore( device->logical_device,
	                    semaphore->semaphore,
	                    device->vulkan_allocator );
	free( semaphore );
}

static void
vk_create_fence( const struct Device* idevice, struct Fence** p )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	FT_INIT_INTERNAL( fence, *p, VulkanFence );

	VkFenceCreateInfo fence_create_info = {};
	fence_create_info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_create_info.pNext             = NULL;
	fence_create_info.flags             = VK_FENCE_CREATE_SIGNALED_BIT;

	VK_ASSERT( vkCreateFence( device->logical_device,
	                          &fence_create_info,
	                          device->vulkan_allocator,
	                          &fence->fence ) );
}

static void
vk_destroy_fence( const struct Device* idevice, struct Fence* ifence )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( fence, ifence, VulkanFence );

	vkDestroyFence( device->logical_device,
	                fence->fence,
	                device->vulkan_allocator );
	free( fence );
}

static void
vk_wait_for_fences( const struct Device* idevice,
                    u32                  count,
                    struct Fence**       ifences )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	ALLOC_STACK_ARRAY( VkFence, fences, count );

	for ( u32 i = 0; i < count; ++i )
	{
		FT_FROM_HANDLE( fence, ifences[ i ], VulkanFence );
		fences[ i ] = fence->fence;
	}

	vkWaitForFences( device->logical_device, count, fences, 1, UINT64_MAX );
}

static void
vk_reset_fences( const struct Device* idevice,
                 u32                  count,
                 struct Fence**       ifences )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	ALLOC_STACK_ARRAY( VkFence, fences, count );

	for ( u32 i = 0; i < count; ++i )
	{
		FT_FROM_HANDLE( fence, ifences[ i ], VulkanFence );
		fences[ i ] = fence->fence;
	}

	vkResetFences( device->logical_device, count, fences );
}

static void
vk_configure_swapchain( const struct VulkanDevice*  device,
                        struct VulkanSwapchain*     swapchain,
                        const struct SwapchainInfo* info )
{
	struct WsiInfo* wsi = info->wsi_info;

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
	ALLOC_STACK_ARRAY( VkPresentModeKHR, present_modes, present_mode_count );
	vkGetPhysicalDeviceSurfacePresentModesKHR( device->physical_device,
	                                           swapchain->surface,
	                                           &present_mode_count,
	                                           present_modes );

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

	//	FT_INFO( "Swapchain present mode: %s",
	//	         string_VkPresentModeKHR( swapchain->present_mode ) );

	// determine present image count
	VkSurfaceCapabilitiesKHR surface_capabilities = {};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device->physical_device,
	                                           swapchain->surface,
	                                           &surface_capabilities );

	// determine swapchain size
	swapchain->interface.width =
	    clamp_u32( info->width,
	               surface_capabilities.minImageExtent.width,
	               surface_capabilities.maxImageExtent.width );
	swapchain->interface.height =
	    clamp_u32( info->height,
	               surface_capabilities.minImageExtent.height,
	               surface_capabilities.maxImageExtent.height );

	swapchain->interface.min_image_count =
	    clamp_u32( info->min_image_count,
	               surface_capabilities.minImageCount,
	               surface_capabilities.maxImageCount );

	/// find best surface format
	u32 surface_format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR( device->physical_device,
	                                      swapchain->surface,
	                                      &surface_format_count,
	                                      NULL );
	ALLOC_STACK_ARRAY( VkSurfaceFormatKHR,
	                   surface_formats,
	                   surface_format_count );
	vkGetPhysicalDeviceSurfaceFormatsKHR( device->physical_device,
	                                      swapchain->surface,
	                                      &surface_format_count,
	                                      surface_formats );

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
vk_create_configured_swapchain( const struct VulkanDevice* device,
                                struct VulkanSwapchain*    swapchain,
                                b32                        resize )
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
			free( image );
		}
	}

	// create new
	VkSwapchainCreateInfoKHR swapchain_create_info = {};
	swapchain_create_info.sType   = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_create_info.pNext   = NULL;
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
	swapchain_create_info.clipped        = 1;
	swapchain_create_info.oldSwapchain   = NULL;

	VK_ASSERT( vkCreateSwapchainKHR( device->logical_device,
	                                 &swapchain_create_info,
	                                 device->vulkan_allocator,
	                                 &swapchain->swapchain ) );

	vkGetSwapchainImagesKHR( device->logical_device,
	                         swapchain->swapchain,
	                         &swapchain->interface.image_count,
	                         NULL );
	ALLOC_STACK_ARRAY( VkImage,
	                   swapchain_images,
	                   swapchain->interface.image_count );
	vkGetSwapchainImagesKHR( device->logical_device,
	                         swapchain->swapchain,
	                         &swapchain->interface.image_count,
	                         swapchain_images );

	if ( !resize )
	{
		swapchain->interface.images =
		    ( struct Image** ) ( calloc( swapchain->interface.image_count,
		                                 sizeof( struct Image* ) ) );
	}

	VkImageViewCreateInfo image_view_create_info = {};
	image_view_create_info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_create_info.pNext    = NULL;
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
		image->interface.sample_count    = 1;
		image->interface.mip_level_count = 1;
		image->interface.layer_count     = 1;
		image->interface.descriptor_type = FT_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

		VK_ASSERT( vkCreateImageView( device->logical_device,
		                              &image_view_create_info,
		                              device->vulkan_allocator,
		                              &image->image_view ) );
	}
}

static void
vk_create_swapchain( const struct Device*        idevice,
                     const struct SwapchainInfo* info,
                     struct Swapchain**          p )
{
	FT_ASSERT( info->wsi_info->create_vulkan_surface );

	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	FT_INIT_INTERNAL( swapchain, *p, VulkanSwapchain );

	vk_configure_swapchain( device, swapchain, info );
	vk_create_configured_swapchain( device, swapchain, 0 );
}

static void
vk_resize_swapchain( const struct Device* idevice,
                     struct Swapchain*    iswapchain,
                     u32                  width,
                     u32                  height )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( swapchain, iswapchain, VulkanSwapchain );

	iswapchain->width  = width;
	iswapchain->height = height;

	u64   iter;
	void* item;
	while ( hashmap_iter( framebuffers[ device->index ], &iter, &item ) )
	{
		struct FramebufferMapItem* fb = item;
		vkDestroyFramebuffer( device->logical_device,
		                      fb->value,
		                      device->vulkan_allocator );
	}
	hashmap_clear( framebuffers[ device->index ], 0 );

	vk_create_configured_swapchain( device, swapchain, 1 );
}

static void
vk_destroy_swapchain( const struct Device* idevice,
                      struct Swapchain*    iswapchain )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( swapchain, iswapchain, VulkanSwapchain );

	for ( u32 i = 0; i < swapchain->interface.image_count; ++i )
	{
		FT_FROM_HANDLE( image, swapchain->interface.images[ i ], VulkanImage );

		vkDestroyImageView( device->logical_device,
		                    image->image_view,
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
vk_create_command_pool( const struct Device*          idevice,
                        const struct CommandPoolInfo* info,
                        struct CommandPool**          p )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	FT_INIT_INTERNAL( command_pool, *p, VulkanCommandPool );

	command_pool->interface.queue = info->queue;

	VkCommandPoolCreateInfo command_pool_create_info = {};
	command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_create_info.pNext = NULL;
	command_pool_create_info.flags =
	    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	command_pool_create_info.queueFamilyIndex = info->queue->family_index;

	VK_ASSERT( vkCreateCommandPool( device->logical_device,
	                                &command_pool_create_info,
	                                device->vulkan_allocator,
	                                &command_pool->command_pool ) );
}

static void
vk_destroy_command_pool( const struct Device* idevice,
                         struct CommandPool*  icommand_pool )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( command_pool, icommand_pool, VulkanCommandPool );

	vkDestroyCommandPool( device->logical_device,
	                      command_pool->command_pool,
	                      device->vulkan_allocator );
	free( command_pool );
}

static void
vk_create_command_buffers( const struct Device*      idevice,
                           const struct CommandPool* icommand_pool,
                           u32                       count,
                           struct CommandBuffer**    icommand_buffers )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( command_pool, icommand_pool, VulkanCommandPool );

	ALLOC_STACK_ARRAY( VkCommandBuffer, buffers, count );

	VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
	command_buffer_allocate_info.sType =
	    VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_allocate_info.pNext       = NULL;
	command_buffer_allocate_info.commandPool = command_pool->command_pool;
	command_buffer_allocate_info.level       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_allocate_info.commandBufferCount = count;

	VK_ASSERT( vkAllocateCommandBuffers( device->logical_device,
	                                     &command_buffer_allocate_info,
	                                     buffers ) );

	// TODO: FIX ME!
	for ( u32 i = 0; i < count; ++i )
	{
		FT_INIT_INTERNAL( cmd, icommand_buffers[ i ], VulkanCommandBuffer );

		cmd->command_buffer  = buffers[ i ];
		cmd->interface.queue = command_pool->interface.queue;
	}
}

static void
vk_free_command_buffers( const struct Device*      idevice,
                         const struct CommandPool* icommand_pool,
                         u32                       count,
                         struct CommandBuffer**    icommand_buffers )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	ALLOC_STACK_ARRAY( VkCommandBuffer, buffers, count );

	for ( u32 i = 0; i < count; ++i )
	{
		FT_FROM_HANDLE( cmd, icommand_buffers[ i ], VulkanCommandBuffer );
		buffers[ i ] = cmd->command_buffer;
	}

	vkFreeCommandBuffers(
	    device->logical_device,
	    ( ( struct VulkanCommandPool* ) ( icommand_pool->handle ) )
	        ->command_pool,
	    count,
	    buffers );
}

static void
vk_destroy_command_buffers( const struct Device*      idevice,
                            const struct CommandPool* icommand_pool,
                            u32                       count,
                            struct CommandBuffer**    icommand_buffers )
{
	( void ) idevice;
	( void ) icommand_pool;

	for ( u32 i = 0; i < count; ++i )
	{
		FT_FROM_HANDLE( cmd, icommand_buffers[ i ], VulkanCommandBuffer );
		free( cmd );
	}
}

static void
vk_begin_command_buffer( const struct CommandBuffer* icmd )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );

	VkCommandBufferBeginInfo command_buffer_begin_info = {};
	command_buffer_begin_info.sType =
	    VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	command_buffer_begin_info.pNext = NULL;
	command_buffer_begin_info.flags =
	    VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	command_buffer_begin_info.pInheritanceInfo = NULL;

	VK_ASSERT( vkBeginCommandBuffer( cmd->command_buffer,
	                                 &command_buffer_begin_info ) );
}

static void
vk_end_command_buffer( const struct CommandBuffer* icmd )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );
	VK_ASSERT( vkEndCommandBuffer( cmd->command_buffer ) );
}

static void
vk_acquire_next_image( const struct Device*    idevice,
                       const struct Swapchain* iswapchain,
                       const struct Semaphore* isemaphore,
                       const struct Fence*     ifence,
                       u32*                    image_index )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( swapchain, iswapchain, VulkanSwapchain );
	FT_FROM_HANDLE( semaphore, isemaphore, VulkanSemaphore );

	VkResult result = vkAcquireNextImageKHR(
	    device->logical_device,
	    swapchain->swapchain,
	    UINT64_MAX,
	    semaphore->semaphore,
	    ifence ? ( ( struct VulkanFence* ) ifence->handle )->fence
	           : VK_NULL_HANDLE,
	    image_index );
	// TODO: check status
	( void ) result;
}

static inline void
vk_create_framebuffer( const struct VulkanDevice*        device,
                       const struct RenderPassBeginInfo* info,
                       VkRenderPass                      render_pass,
                       VkFramebuffer*                    p )
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

	VkFramebufferCreateInfo framebuffer_create_info = {};
	framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebuffer_create_info.pNext = NULL;
	framebuffer_create_info.flags = 0;
	framebuffer_create_info.renderPass      = render_pass;
	framebuffer_create_info.attachmentCount = attachment_count;
	framebuffer_create_info.pAttachments    = image_views;
	framebuffer_create_info.width           = info->width;
	framebuffer_create_info.height          = info->height;
	framebuffer_create_info.layers          = 1;

	VK_ASSERT( vkCreateFramebuffer( device->logical_device,
	                                &framebuffer_create_info,
	                                device->vulkan_allocator,
	                                p ) );
}

static inline void
vk_create_render_pass( const struct VulkanDevice*        device,
                       const struct RenderPassBeginInfo* info,
                       VkRenderPass*                     p )
{
	u32 attachments_count = info->color_attachment_count;

	VkAttachmentDescription
	                      attachment_descriptions[ MAX_ATTACHMENTS_COUNT + 2 ];
	VkAttachmentReference color_attachment_references[ MAX_ATTACHMENTS_COUNT ];
	VkAttachmentReference depth_attachment_reference = {};

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
	VkSubpassDescription subpass_description = {};
	subpass_description.flags                = 0;
	subpass_description.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass_description.inputAttachmentCount = 0;
	subpass_description.pInputAttachments    = NULL;
	subpass_description.colorAttachmentCount = info->color_attachment_count;
	subpass_description.pColorAttachments =
	    attachments_count ? color_attachment_references : NULL;
	subpass_description.pDepthStencilAttachment =
	    info->depth_stencil ? &depth_attachment_reference : NULL;
	subpass_description.pResolveAttachments     = NULL;
	subpass_description.preserveAttachmentCount = 0;
	subpass_description.pPreserveAttachments    = NULL;

	VkRenderPassCreateInfo render_pass_create_info = {};
	render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.pNext = NULL;
	render_pass_create_info.flags = 0;
	render_pass_create_info.attachmentCount = attachments_count;
	render_pass_create_info.pAttachments =
	    attachments_count ? attachment_descriptions : NULL;
	render_pass_create_info.subpassCount    = 1;
	render_pass_create_info.pSubpasses      = &subpass_description;
	render_pass_create_info.dependencyCount = 0;
	render_pass_create_info.pDependencies   = NULL;

	VK_ASSERT( vkCreateRenderPass( device->logical_device,
	                               &render_pass_create_info,
	                               device->vulkan_allocator,
	                               p ) );
}

static inline VkRenderPass
get_render_pass( const struct RenderPassBeginInfo* info )
{
	FT_FROM_HANDLE( device, info->device, VulkanDevice );

	struct RenderPassMapItem* it =
	    hashmap_get( passes[ device->index ],
	                 &( struct RenderPassMapItem ) { .info = *info } );

	if ( it != NULL )
	{
		return it->value;
	}
	else
	{
		VkRenderPass render_pass;
		vk_create_render_pass( device, info, &render_pass );
		hashmap_set( passes[ device->index ],
		             &( struct RenderPassMapItem ) { .info  = *info,
		                                             .value = render_pass } );
		return render_pass;
	}
}

static inline VkFramebuffer
get_framebuffer( VkRenderPass                      render_pass,
                 const struct RenderPassBeginInfo* info )
{
	FT_FROM_HANDLE( device, info->device, VulkanDevice );

	struct FramebufferMapItem* it =
	    hashmap_get( framebuffers[ device->index ],
	                 &( struct FramebufferMapItem ) { .info = *info } );

	if ( it != NULL )
	{
		return it->value;
	}
	else
	{
		VkFramebuffer framebuffer;
		vk_create_framebuffer( device, info, render_pass, &framebuffer );
		hashmap_set( framebuffers[ device->index ],
		             &( struct FramebufferMapItem ) { .info  = *info,
		                                              .value = framebuffer } );

		return framebuffer;
	}
}

static inline void
vk_create_module( const struct VulkanDevice*     device,
                  struct VulkanShader*           shader,
                  enum ShaderStage               stage,
                  const struct ShaderModuleInfo* info )
{
	if ( info->bytecode )
	{
		VkShaderModuleCreateInfo shader_create_info = {};
		shader_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shader_create_info.pNext = NULL;
		shader_create_info.flags = 0;
		shader_create_info.codeSize = info->bytecode_size;
		shader_create_info.pCode    = ( const u32* ) ( info->bytecode );

		VK_ASSERT( vkCreateShaderModule( device->logical_device,
		                                 &shader_create_info,
		                                 device->vulkan_allocator,
		                                 &shader->shaders[ stage ] ) );
	}
}

static void
vk_create_shader( const struct Device* idevice,
                  struct ShaderInfo*   info,
                  struct Shader**      p )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	FT_INIT_INTERNAL( shader, *p, VulkanShader );

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

static inline void
vk_destroy_module( const struct VulkanDevice* device,
                   enum ShaderStage           stage,
                   struct VulkanShader*       shader )
{
	if ( shader->shaders[ stage ] )
	{
		vkDestroyShaderModule( device->logical_device,
		                       shader->shaders[ stage ],
		                       device->vulkan_allocator );
	}
}

static void
vk_destroy_shader( const struct Device* idevice, struct Shader* ishader )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( shader, ishader, VulkanShader );

	vk_destroy_module( device, FT_SHADER_STAGE_COMPUTE, shader );
	vk_destroy_module( device, FT_SHADER_STAGE_VERTEX, shader );
	vk_destroy_module( device, FT_SHADER_STAGE_TESSELLATION_CONTROL, shader );
	vk_destroy_module( device,
	                   FT_SHADER_STAGE_TESSELLATION_EVALUATION,
	                   shader );
	vk_destroy_module( device, FT_SHADER_STAGE_GEOMETRY, shader );
	vk_destroy_module( device, FT_SHADER_STAGE_FRAGMENT, shader );

	if ( shader->interface.reflect_data.binding_count )
	{
		hashmap_free( shader->interface.reflect_data.binding_map );
		free( shader->interface.reflect_data.bindings );
	}

	free( shader );
}

static void
vk_create_descriptor_set_layout( const struct Device*         idevice,
                                 struct Shader*               ishader,
                                 struct DescriptorSetLayout** p )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	FT_INIT_INTERNAL( descriptor_set_layout, *p, VulkanDescriptorSetLayout );

	// copy reflection data because shader can be destroyed earlier
	ReflectionData reflect_data = {};
	reflect_data.binding_count  = ishader->reflect_data.binding_count;
	reflect_data.binding_map    = hashmap_new( sizeof( struct BindingMapItem ),
                                            0,
                                            0,
                                            0,
                                            binding_map_hash,
                                            binding_map_compare,
                                            NULL,
                                            NULL );
	if ( reflect_data.binding_count != 0 )
	{
		u64   iter = 0;
		void* item;
		while (
		    hashmap_iter( ishader->reflect_data.binding_map, &iter, &item ) )
		{
			hashmap_set( reflect_data.binding_map, item );
		}

		ALLOC_HEAP_ARRAY( struct Binding,
		                  bindings,
		                  ishader->reflect_data.binding_count );
		reflect_data.bindings = bindings;
		for ( u32 i = 0; i < ishader->reflect_data.binding_count; ++i )
		{
			reflect_data.bindings[ i ] = ishader->reflect_data.bindings[ i ];
		}
	}
	descriptor_set_layout->interface.reflection_data = reflect_data;

	// count bindings in all shaders
	u32                      binding_counts[ MAX_SET_COUNT ] = { 0 };
	VkDescriptorBindingFlags binding_flags[ MAX_SET_COUNT ]
	                                      [ MAX_DESCRIPTOR_BINDING_COUNT ] = {};
	// collect all bindings
	VkDescriptorSetLayoutBinding bindings[ MAX_SET_COUNT ]
	                                     [ MAX_DESCRIPTOR_BINDING_COUNT ];

	u32 set_count = 0;

	ReflectionData* reflection =
	    &descriptor_set_layout->interface.reflection_data;

	for ( u32 b = 0; b < reflection->binding_count; ++b )
	{
		struct Binding* binding       = &reflection->bindings[ b ];
		u32             set           = binding->set;
		u32             binding_count = binding_counts[ set ];

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

	for ( u32 set = 0; set < set_count; ++set )
	{
		if ( binding_counts[ set ] > 0 )
		{
			VkDescriptorSetLayoutBindingFlagsCreateInfo
			    binding_flags_create_info = {};
			binding_flags_create_info.sType =
			    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
			binding_flags_create_info.pNext         = NULL;
			binding_flags_create_info.bindingCount  = binding_counts[ set ];
			binding_flags_create_info.pBindingFlags = binding_flags[ set ];

			VkDescriptorSetLayoutCreateInfo
			    descriptor_set_layout_create_info = {};
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

			descriptor_set_layout->descriptor_set_layout_count++;
		}
	}
}

static void
vk_destroy_descriptor_set_layout( const struct Device*        idevice,
                                  struct DescriptorSetLayout* ilayout )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( layout, ilayout, VulkanDescriptorSetLayout );

	for ( u32 i = 0; i < layout->descriptor_set_layout_count; ++i )
	{
		if ( layout->descriptor_set_layouts[ i ] )
		{
			vkDestroyDescriptorSetLayout( device->logical_device,
			                              layout->descriptor_set_layouts[ i ],
			                              device->vulkan_allocator );
		}
	}

	if ( layout->interface.reflection_data.binding_count )
	{
		hashmap_free( layout->interface.reflection_data.binding_map );
		free( layout->interface.reflection_data.bindings );
	}

	free( layout );
}

static void
vk_create_compute_pipeline( const struct Device*       idevice,
                            const struct PipelineInfo* info,
                            struct Pipeline**          p )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( shader, info->shader, VulkanShader );
	FT_FROM_HANDLE( dsl,
	                info->descriptor_set_layout,
	                VulkanDescriptorSetLayout );

	FT_INIT_INTERNAL( pipeline, *p, VulkanPipeline );

	pipeline->interface.type = FT_PIPELINE_TYPE_COMPUTE;

	VkPipelineShaderStageCreateInfo shader_stage_create_info;
	shader_stage_create_info.sType =
	    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage_create_info.pNext = NULL;
	shader_stage_create_info.flags = 0;
	shader_stage_create_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	shader_stage_create_info.module =
	    shader->shaders[ FT_SHADER_STAGE_COMPUTE ];
	shader_stage_create_info.pName               = "main";
	shader_stage_create_info.pSpecializationInfo = NULL;

	VkPushConstantRange push_constant_range = {};
	push_constant_range.size                = MAX_PUSH_CONSTANT_RANGE;
	push_constant_range.stageFlags          = VK_SHADER_STAGE_VERTEX_BIT |
	                                 VK_SHADER_STAGE_COMPUTE_BIT |
	                                 VK_SHADER_STAGE_FRAGMENT_BIT;

	VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
	pipeline_layout_create_info.sType =
	    VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_create_info.setLayoutCount =
	    dsl->descriptor_set_layout_count;
	pipeline_layout_create_info.pSetLayouts =
	    ( ( struct VulkanDescriptorSetLayout* )
	          info->descriptor_set_layout->handle )
	        ->descriptor_set_layouts;
	pipeline_layout_create_info.pushConstantRangeCount = 1;
	pipeline_layout_create_info.pPushConstantRanges    = &push_constant_range;

	VK_ASSERT( vkCreatePipelineLayout( device->logical_device,
	                                   &pipeline_layout_create_info,
	                                   NULL,
	                                   &pipeline->pipeline_layout ) );

	VkComputePipelineCreateInfo compute_pipeline_create_info = {};
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

static void
vk_create_graphics_pipeline( const struct Device*       idevice,
                             const struct PipelineInfo* info,
                             struct Pipeline**          p )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( shader, info->shader, VulkanShader );
	FT_FROM_HANDLE( dsl,
	                info->descriptor_set_layout,
	                VulkanDescriptorSetLayout );

	FT_INIT_INTERNAL( pipeline, *p, VulkanPipeline );

	struct RenderPassBeginInfo render_pass_info = {};
	render_pass_info.color_attachment_count     = info->color_attachment_count;
	ALLOC_STACK_ARRAY( struct Image,
	                   color_images,
	                   info->color_attachment_count );
	for ( u32 i = 0; i < info->color_attachment_count; ++i )
	{
		color_images[ i ].format       = info->color_attachment_formats[ i ];
		color_images[ i ].sample_count = info->sample_count;
		render_pass_info.color_attachments[ i ] = &color_images[ i ];
		render_pass_info.color_attachment_load_ops[ i ] =
		    FT_ATTACHMENT_LOAD_OP_DONT_CARE;
		render_pass_info.color_image_states[ i ] =
		    FT_RESOURCE_STATE_COLOR_ATTACHMENT;
	}

	struct Image depth_image;
	if ( info->depth_stencil_format != FT_FORMAT_UNDEFINED )
	{
		depth_image.format             = info->depth_stencil_format;
		depth_image.sample_count       = info->sample_count;
		render_pass_info.depth_stencil = &depth_image;
		render_pass_info.depth_stencil_load_op =
		    FT_ATTACHMENT_LOAD_OP_DONT_CARE;
		render_pass_info.depth_stencil_state =
		    FT_RESOURCE_STATE_DEPTH_STENCIL_WRITE;
	}

	VkRenderPass render_pass;
	vk_create_render_pass( device, &render_pass_info, &render_pass );

	pipeline->interface.type = FT_PIPELINE_TYPE_GRAPHICS;

	u32 shader_stage_count = 0;
	VkPipelineShaderStageCreateInfo
	    shader_stage_create_infos[ FT_SHADER_STAGE_COUNT ];

	for ( u32 i = 0; i < FT_SHADER_STAGE_COUNT; ++i )
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

	const struct VertexLayout* vertex_layout = &info->vertex_layout;

	VkVertexInputBindingDescription
	    binding_descriptions[ MAX_VERTEX_BINDING_COUNT ];
	for ( u32 i = 0; i < vertex_layout->binding_info_count; ++i )
	{
		binding_descriptions[ i ].binding =
		    vertex_layout->binding_infos[ i ].binding;
		binding_descriptions[ i ].stride =
		    vertex_layout->binding_infos[ i ].stride;
		binding_descriptions[ i ].inputRate = to_vk_vertex_input_rate(
		    vertex_layout->binding_infos[ i ].input_rate );
	}

	VkVertexInputAttributeDescription
	    attribute_descriptions[ MAX_VERTEX_ATTRIBUTE_COUNT ];
	for ( u32 i = 0; i < vertex_layout->attribute_info_count; ++i )
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

	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {};
	vertex_input_state_create_info.sType =
	    VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_state_create_info.vertexBindingDescriptionCount =
	    vertex_layout->binding_info_count;
	vertex_input_state_create_info.pVertexBindingDescriptions =
	    binding_descriptions;
	vertex_input_state_create_info.vertexAttributeDescriptionCount =
	    vertex_layout->attribute_info_count;
	vertex_input_state_create_info.pVertexAttributeDescriptions =
	    attribute_descriptions;

	VkPipelineInputAssemblyStateCreateInfo
	    input_assembly_state_create_info = {};
	input_assembly_state_create_info.sType =
	    VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_state_create_info.topology =
	    to_vk_primitive_topology( info->topology );
	input_assembly_state_create_info.primitiveRestartEnable = 0;

	// Dynamic states
	VkViewport viewport = {};
	VkRect2D   scissor  = {};

	VkPipelineViewportStateCreateInfo viewport_state_create_info = {};
	viewport_state_create_info.sType =
	    VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state_create_info.viewportCount = 1;
	viewport_state_create_info.pViewports    = &viewport;
	viewport_state_create_info.scissorCount  = 1;
	viewport_state_create_info.pScissors     = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {};
	rasterization_state_create_info.sType =
	    VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_state_create_info.polygonMode =
	    to_vk_polygon_mode( info->rasterizer_info.polygon_mode );
	rasterization_state_create_info.cullMode =
	    to_vk_cull_mode( info->rasterizer_info.cull_mode );
	rasterization_state_create_info.frontFace =
	    to_vk_front_face( info->rasterizer_info.front_face );
	rasterization_state_create_info.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {};
	multisample_state_create_info.sType =
	    VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_state_create_info.rasterizationSamples =
	    to_vk_sample_count( info->sample_count );
	multisample_state_create_info.sampleShadingEnable = VK_FALSE;
	multisample_state_create_info.minSampleShading    = 1.0f;

	ALLOC_STACK_ARRAY( VkPipelineColorBlendAttachmentState,
	                   attachment_states,
	                   info->color_attachment_count );
	for ( u32 i = 0; i < info->color_attachment_count; ++i )
	{
		VkPipelineColorBlendAttachmentState attachment_state = {};
		attachment_states[ i ]                               = attachment_state;
		attachment_states[ i ].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		attachment_states[ i ].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		attachment_states[ i ].colorBlendOp        = VK_BLEND_OP_ADD;
		attachment_states[ i ].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		attachment_states[ i ].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		attachment_states[ i ].colorWriteMask =
		    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	}

	VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {};
	color_blend_state_create_info.sType =
	    VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_state_create_info.logicOpEnable = 0;
	color_blend_state_create_info.logicOp       = VK_LOGIC_OP_COPY;
	color_blend_state_create_info.attachmentCount =
	    info->color_attachment_count;
	color_blend_state_create_info.pAttachments = attachment_states;

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {};
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

	enum
	{
		DYNAMIC_STATE_COUNT = 2
	};

	VkDynamicState dynamic_states[ DYNAMIC_STATE_COUNT ] = {
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_VIEWPORT
	};

	VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {};
	dynamic_state_create_info.sType =
	    VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_create_info.dynamicStateCount = DYNAMIC_STATE_COUNT;
	dynamic_state_create_info.pDynamicStates    = dynamic_states;

	VkPushConstantRange push_constant_range = {};
	push_constant_range.size                = MAX_PUSH_CONSTANT_RANGE;
	push_constant_range.stageFlags          = VK_SHADER_STAGE_VERTEX_BIT |
	                                 VK_SHADER_STAGE_COMPUTE_BIT |
	                                 VK_SHADER_STAGE_FRAGMENT_BIT;

	VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
	pipeline_layout_create_info.sType =
	    VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_create_info.setLayoutCount =
	    dsl->descriptor_set_layout_count;
	pipeline_layout_create_info.pSetLayouts =
	    ( ( struct VulkanDescriptorSetLayout* )
	          info->descriptor_set_layout->handle )
	        ->descriptor_set_layouts;
	pipeline_layout_create_info.pushConstantRangeCount = 1;
	pipeline_layout_create_info.pPushConstantRanges    = &push_constant_range;

	VK_ASSERT( vkCreatePipelineLayout( device->logical_device,
	                                   &pipeline_layout_create_info,
	                                   device->vulkan_allocator,
	                                   &pipeline->pipeline_layout ) );

	VkGraphicsPipelineCreateInfo pipeline_create_info = {};
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
	pipeline_create_info.renderPass          = render_pass;

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
vk_destroy_pipeline( const struct Device* idevice, struct Pipeline* ipipeline )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( pipeline, ipipeline, VulkanPipeline );

	vkDestroyPipelineLayout( device->logical_device,
	                         pipeline->pipeline_layout,
	                         device->vulkan_allocator );
	vkDestroyPipeline( device->logical_device,
	                   pipeline->pipeline,
	                   device->vulkan_allocator );
	free( pipeline );
}

static void
vk_create_buffer( const struct Device*     idevice,
                  const struct BufferInfo* info,
                  struct Buffer**          p )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	FT_INIT_INTERNAL( buffer, *p, VulkanBuffer );

	buffer->interface.size            = info->size;
	buffer->interface.descriptor_type = info->descriptor_type;
	buffer->interface.memory_usage    = info->memory_usage;

	VmaAllocationCreateInfo allocation_create_info = {};
	allocation_create_info.usage =
	    determine_vma_memory_usage( info->memory_usage );

	VkBufferCreateInfo buffer_create_info = {};
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.pNext = NULL;
	buffer_create_info.flags = 0;
	buffer_create_info.size  = info->size;
	buffer_create_info.usage =
	    determine_vk_buffer_usage( info->descriptor_type, info->memory_usage );
	buffer_create_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
	buffer_create_info.queueFamilyIndexCount = 0;
	buffer_create_info.pQueueFamilyIndices   = NULL;

	VK_ASSERT( vmaCreateBuffer( device->memory_allocator,
	                            &buffer_create_info,
	                            &allocation_create_info,
	                            &buffer->buffer,
	                            &buffer->allocation,
	                            NULL ) );
}

static void
vk_destroy_buffer( const struct Device* idevice, struct Buffer* ibuffer )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( buffer, ibuffer, VulkanBuffer );

	vmaDestroyBuffer( device->memory_allocator,
	                  buffer->buffer,
	                  buffer->allocation );
	free( buffer );
}

static void
vk_create_sampler( const struct Device*      idevice,
                   const struct SamplerInfo* info,
                   struct Sampler**          p )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	FT_INIT_INTERNAL( sampler, *p, VulkanSampler );

	VkSamplerCreateInfo sampler_create_info = {};
	sampler_create_info.sType     = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_create_info.pNext     = NULL;
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
	sampler_create_info.unnormalizedCoordinates = 0;

	VK_ASSERT( vkCreateSampler( device->logical_device,
	                            &sampler_create_info,
	                            device->vulkan_allocator,
	                            &sampler->sampler ) );
}

static void
vk_destroy_sampler( const struct Device* idevice, struct Sampler* isampler )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( sampler, isampler, VulkanSampler );

	vkDestroySampler( device->logical_device,
	                  sampler->sampler,
	                  device->vulkan_allocator );
	free( sampler );
}

static void
vk_create_image( const struct Device*    idevice,
                 const struct ImageInfo* info,
                 struct Image**          p )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	FT_INIT_INTERNAL( image, *p, VulkanImage );

	VmaAllocationCreateInfo allocation_create_info = {};
	allocation_create_info.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;

	VkImageCreateInfo image_create_info = {};
	image_create_info.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.pNext             = NULL;
	image_create_info.flags             = 0;
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
	image_create_info.pQueueFamilyIndices   = NULL;
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
	                           NULL ) );

	image->interface.width           = info->width;
	image->interface.height          = info->height;
	image->interface.depth           = info->depth;
	image->interface.format          = info->format;
	image->interface.sample_count    = info->sample_count;
	image->interface.mip_level_count = info->mip_levels;
	image->interface.layer_count     = info->layer_count;
	image->interface.descriptor_type = info->descriptor_type;

	// TODO: fill properly
	VkImageViewCreateInfo image_view_create_info = {};
	image_view_create_info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_create_info.pNext    = NULL;
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

static void
vk_destroy_image( const struct Device* idevice, struct Image* iimage )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( image, iimage, VulkanImage );

	vkDestroyImageView( device->logical_device,
	                    image->image_view,
	                    device->vulkan_allocator );
	vmaDestroyImage( device->memory_allocator,
	                 image->image,
	                 image->allocation );
	free( image );
}

static void
vk_create_descriptor_set( const struct Device*            idevice,
                          const struct DescriptorSetInfo* info,
                          struct DescriptorSet**          p )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	FT_INIT_INTERNAL( descriptor_set, *p, VulkanDescriptorSet );

	descriptor_set->interface.layout = info->descriptor_set_layout;

	VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
	descriptor_set_allocate_info.sType =
	    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptor_set_allocate_info.pNext              = NULL;
	descriptor_set_allocate_info.descriptorPool     = device->descriptor_pool;
	descriptor_set_allocate_info.descriptorSetCount = 1;
	descriptor_set_allocate_info.pSetLayouts =
	    &( ( struct VulkanDescriptorSetLayout* )
	           info->descriptor_set_layout->handle )
	         ->descriptor_set_layouts[ info->set ];

	VK_ASSERT( vkAllocateDescriptorSets( device->logical_device,
	                                     &descriptor_set_allocate_info,
	                                     &descriptor_set->descriptor_set ) );
}

static void
vk_destroy_descriptor_set( const struct Device*  idevice,
                           struct DescriptorSet* iset )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( set, iset, VulkanDescriptorSet );

	vkFreeDescriptorSets( device->logical_device,
	                      device->descriptor_pool,
	                      1,
	                      &set->descriptor_set );
	free( set );
}

static void
vk_update_descriptor_set( const struct Device*          idevice,
                          struct DescriptorSet*         iset,
                          u32                           count,
                          const struct DescriptorWrite* writes )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );
	FT_FROM_HANDLE( set, iset, VulkanDescriptorSet );

	// TODO: rewrite
	ALLOC_HEAP_ARRAY( VkDescriptorBufferInfo*, buffer_updates, count );
	u32 buffer_update_idx = 0;
	ALLOC_HEAP_ARRAY( VkDescriptorImageInfo*, image_updates, count );
	u32 image_update_idx = 0;
	ALLOC_HEAP_ARRAY( VkWriteDescriptorSet, descriptor_writes, count );

	u32 write = 0;

	for ( u32 i = 0; i < count; ++i )
	{
		const struct DescriptorWrite* descriptor_write = &writes[ i ];

		ReflectionData* reflection = &set->interface.layout->reflection_data;

		struct BindingMapItem item;
		memset( item.name, '\0', MAX_BINDING_NAME_LENGTH );
		strcpy( item.name, descriptor_write->descriptor_name );
		struct BindingMapItem* it =
		    hashmap_get( reflection->binding_map, &item );

		FT_ASSERT( it != NULL );

		const struct Binding* binding = &reflection->bindings[ it->value ];

		VkWriteDescriptorSet* write_descriptor_set =
		    &descriptor_writes[ write++ ];
		//		write_descriptor_set       = {};
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
			    ( VkDescriptorBufferInfo* ) calloc(
			        sizeof( VkDescriptorBufferInfo ),
			        descriptor_write->descriptor_count );
			buffer_updates[ buffer_update_idx++ ] = buffer_infos;

			for ( u32 j = 0; j < descriptor_write->descriptor_count; ++j )
			{
				buffer_infos[ j ].buffer =
				    ( ( struct VulkanBuffer* ) descriptor_write
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
			    ( VkDescriptorImageInfo* ) calloc(
			        sizeof( VkDescriptorImageInfo ),
			        descriptor_write->descriptor_count );
			image_updates[ image_update_idx++ ] = image_infos;

			for ( u32 j = 0; j < descriptor_write->descriptor_count; ++j )
			{
				struct ImageDescriptor* descriptor =
				    &descriptor_write->image_descriptors[ j ];

				FT_ASSERT( descriptor->image );

				image_infos[ j ].imageLayout =
				    determine_image_layout( descriptor->resource_state );
				image_infos[ j ].imageView =
				    ( ( struct VulkanImage* ) descriptor->image->handle )
				        ->image_view;
				image_infos[ j ].sampler = NULL;
			}

			write_descriptor_set->pImageInfo = image_infos;
		}
		else
		{
			VkDescriptorImageInfo* image_infos =
			    ( VkDescriptorImageInfo* ) calloc(
			        sizeof( VkDescriptorImageInfo ),
			        descriptor_write->descriptor_count );
			image_updates[ image_update_idx++ ] = image_infos;

			for ( u32 j = 0; j < descriptor_write->descriptor_count; ++j )
			{
				struct SamplerDescriptor* descriptor =
				    &descriptor_write->sampler_descriptors[ j ];

				FT_ASSERT( descriptor->sampler );

				image_infos[ j ].sampler =
				    ( ( struct VulkanSampler* ) descriptor->sampler->handle )
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

	for ( u32 i = 0; i < count; ++i )
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
vk_cmd_begin_render_pass( const struct CommandBuffer*       icmd,
                          const struct RenderPassBeginInfo* info )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );

	VkRenderPass  render_pass = get_render_pass( info );
	VkFramebuffer framebuffer = get_framebuffer( render_pass, info );

	static VkClearValue clear_values[ MAX_ATTACHMENTS_COUNT + 1 ];
	u32                 clear_value_count = info->color_attachment_count;

	for ( u32 i = 0; i < info->color_attachment_count; ++i )
	{
		VkClearValue clear_value = {};
		clear_values[ i ]        = clear_value;
		clear_values[ i ].color.float32[ 0 ] =
		    info->clear_values[ i ].color[ 0 ];
		clear_values[ i ].color.float32[ 1 ] =
		    info->clear_values[ i ].color[ 1 ];
		clear_values[ i ].color.float32[ 2 ] =
		    info->clear_values[ i ].color[ 2 ];
		clear_values[ i ].color.float32[ 3 ] =
		    info->clear_values[ i ].color[ 3 ];
	}

	if ( info->depth_stencil )
	{
		clear_value_count++;
		u32          idx         = info->color_attachment_count;
		VkClearValue clear_value = {};
		clear_values[ idx ]      = clear_value;
		clear_values[ idx ].depthStencil.depth =
		    info->clear_values[ idx ].depth_stencil.depth;
		clear_values[ idx ].depthStencil.stencil =
		    info->clear_values[ idx ].depth_stencil.stencil;
	}

	VkRenderPassBeginInfo render_pass_begin_info = {};
	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.pNext = NULL;
	render_pass_begin_info.renderPass               = render_pass;
	render_pass_begin_info.framebuffer              = framebuffer;
	render_pass_begin_info.renderArea.extent.width  = info->width;
	render_pass_begin_info.renderArea.extent.height = info->height;
	render_pass_begin_info.renderArea.offset.x      = 0;
	render_pass_begin_info.renderArea.offset.y      = 0;
	render_pass_begin_info.clearValueCount          = clear_value_count;
	render_pass_begin_info.pClearValues             = clear_values;

	vkCmdBeginRenderPass( cmd->command_buffer,
	                      &render_pass_begin_info,
	                      VK_SUBPASS_CONTENTS_INLINE );
}

static void
vk_cmd_end_render_pass( const struct CommandBuffer* icmd )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );
	vkCmdEndRenderPass( cmd->command_buffer );
}

static void
vk_cmd_barrier( const struct CommandBuffer* icmd,
                u32                         memory_barriers_count,
                const struct MemoryBarrier* memory_barriers,
                u32                         buffer_barriers_count,
                const struct BufferBarrier* buffer_barriers,
                u32                         image_barriers_count,
                const struct ImageBarrier*  image_barriers )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );

	ALLOC_STACK_ARRAY( VkBufferMemoryBarrier,
	                   buffer_memory_barriers,
	                   buffer_barriers_count );
	ALLOC_STACK_ARRAY( VkImageMemoryBarrier,
	                   image_memory_barriers,
	                   image_barriers_count );

	// TODO: Queues
	VkAccessFlags src_access = ( VkAccessFlags ) 0;
	VkAccessFlags dst_access = ( VkAccessFlags ) 0;

	for ( u32 i = 0; i < buffer_barriers_count; ++i )
	{
		FT_ASSERT( buffer_barriers[ i ].buffer );

		FT_FROM_HANDLE( buffer, buffer_barriers[ i ].buffer, VulkanBuffer );

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

	for ( u32 i = 0; i < image_barriers_count; ++i )
	{
		FT_ASSERT( image_barriers[ i ].image );

		FT_FROM_HANDLE( image, image_barriers[ i ].image, VulkanImage );

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
vk_cmd_set_scissor( const struct CommandBuffer* icmd,
                    i32                         x,
                    i32                         y,
                    u32                         width,
                    u32                         height )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );

	VkRect2D scissor      = {};
	scissor.offset.x      = x;
	scissor.offset.y      = y;
	scissor.extent.width  = width;
	scissor.extent.height = height;
	vkCmdSetScissor( cmd->command_buffer, 0, 1, &scissor );
}

static void
vk_cmd_set_viewport( const struct CommandBuffer* icmd,
                     f32                         x,
                     f32                         y,
                     f32                         width,
                     f32                         height,
                     f32                         min_depth,
                     f32                         max_depth )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );

	VkViewport viewport = {};
	viewport.x          = x;
	viewport.y          = y + height;
	viewport.width      = width;
	viewport.height     = -height;
	viewport.minDepth   = min_depth;
	viewport.maxDepth   = max_depth;

	vkCmdSetViewport( cmd->command_buffer, 0, 1, &viewport );
}

static void
vk_cmd_bind_pipeline( const struct CommandBuffer* icmd,
                      const struct Pipeline*      ipipeline )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );
	FT_FROM_HANDLE( pipeline, ipipeline, VulkanPipeline );

	vkCmdBindPipeline( cmd->command_buffer,
	                   to_vk_pipeline_bind_point( pipeline->interface.type ),
	                   pipeline->pipeline );
}

static void
vk_cmd_draw( const struct CommandBuffer* icmd,
             u32                         vertex_count,
             u32                         instance_count,
             u32                         first_vertex,
             u32                         first_instance )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );

	vkCmdDraw( cmd->command_buffer,
	           vertex_count,
	           instance_count,
	           first_vertex,
	           first_instance );
}

static void
vk_cmd_draw_indexed( const struct CommandBuffer* icmd,
                     u32                         index_count,
                     u32                         instance_count,
                     u32                         first_index,
                     i32                         vertex_offset,
                     u32                         first_instance )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );

	vkCmdDrawIndexed( cmd->command_buffer,
	                  index_count,
	                  instance_count,
	                  first_index,
	                  vertex_offset,
	                  first_instance );
}

static void
vk_cmd_bind_vertex_buffer( const struct CommandBuffer* icmd,
                           const struct Buffer*        ibuffer,
                           const u64                   offset )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );
	FT_FROM_HANDLE( buffer, ibuffer, VulkanBuffer );

	vkCmdBindVertexBuffers( cmd->command_buffer,
	                        0,
	                        1,
	                        &buffer->buffer,
	                        &offset );
}

static void
vk_cmd_bind_index_buffer_u16( const struct CommandBuffer* icmd,
                              const struct Buffer*        ibuffer,
                              const u64                   offset )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );
	FT_FROM_HANDLE( buffer, ibuffer, VulkanBuffer );

	vkCmdBindIndexBuffer( cmd->command_buffer,
	                      buffer->buffer,
	                      offset,
	                      VK_INDEX_TYPE_UINT16 );
}

static void
vk_cmd_bind_index_buffer_u32( const struct CommandBuffer* icmd,
                              const struct Buffer*        ibuffer,
                              u64                         offset )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );
	FT_FROM_HANDLE( buffer, ibuffer, VulkanBuffer );

	vkCmdBindIndexBuffer( cmd->command_buffer,
	                      buffer->buffer,
	                      offset,
	                      VK_INDEX_TYPE_UINT32 );
}

static void
vk_cmd_copy_buffer( const struct CommandBuffer* icmd,
                    const struct Buffer*        isrc,
                    u64                         src_offset,
                    struct Buffer*              idst,
                    u64                         dst_offset,
                    u64                         size )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );
	FT_FROM_HANDLE( src, isrc, VulkanBuffer );
	FT_FROM_HANDLE( dst, idst, VulkanBuffer );

	VkBufferCopy buffer_copy = {};
	buffer_copy.srcOffset    = src_offset;
	buffer_copy.dstOffset    = dst_offset;
	buffer_copy.size         = size;

	vkCmdCopyBuffer( cmd->command_buffer,
	                 src->buffer,
	                 dst->buffer,
	                 1,
	                 &buffer_copy );
}

static void
vk_cmd_copy_buffer_to_image( const struct CommandBuffer* icmd,
                             const struct Buffer*        isrc,
                             u64                         src_offset,
                             struct Image*               idst )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );
	FT_FROM_HANDLE( src, isrc, VulkanBuffer );
	FT_FROM_HANDLE( dst, idst, VulkanImage );

	VkImageSubresourceLayers dst_layers = get_image_subresource_layers( dst );

	VkOffset3D offset = { 0, 0, 0 };
	VkExtent3D extent = { dst->interface.width, dst->interface.height, 1 };

	VkBufferImageCopy buffer_to_image_copy_info = {};
	buffer_to_image_copy_info.bufferOffset      = src_offset;
	buffer_to_image_copy_info.bufferImageHeight = 0;
	buffer_to_image_copy_info.bufferRowLength   = 0;
	buffer_to_image_copy_info.imageSubresource  = dst_layers;
	buffer_to_image_copy_info.imageOffset       = offset;
	buffer_to_image_copy_info.imageExtent       = extent;

	vkCmdCopyBufferToImage(
	    cmd->command_buffer,
	    src->buffer,
	    dst->image,
	    determine_image_layout( FT_RESOURCE_STATE_TRANSFER_DST ),
	    1,
	    &buffer_to_image_copy_info );
}

static void
vk_cmd_dispatch( const struct CommandBuffer* icmd,
                 u32                         group_count_x,
                 u32                         group_count_y,
                 u32                         group_count_z )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );

	vkCmdDispatch( cmd->command_buffer,
	               group_count_x,
	               group_count_y,
	               group_count_z );
}

static void
vk_cmd_push_constants( const struct CommandBuffer* icmd,
                       const struct Pipeline*      ipipeline,
                       u32                         offset,
                       u32                         size,
                       const void*                 data )
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

static void
vk_cmd_clear_color_image( const struct CommandBuffer* icmd,
                          struct Image*               iimage,
                          float                       color[ 4 ] )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );
	FT_FROM_HANDLE( image, iimage, VulkanImage );

	VkClearColorValue clear_color = {};
	clear_color.float32[ 0 ]      = color[ 0 ];
	clear_color.float32[ 1 ]      = color[ 1 ];
	clear_color.float32[ 2 ]      = color[ 2 ];
	clear_color.float32[ 3 ]      = color[ 3 ];

	VkImageSubresourceRange range = get_image_subresource_range( image );

	vkCmdClearColorImage( cmd->command_buffer,
	                      image->image,
	                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	                      &clear_color,
	                      1,
	                      &range );
}

static void
vk_cmd_draw_indexed_indirect( const struct CommandBuffer* icmd,
                              const struct Buffer*        ibuffer,
                              u64                         offset,
                              u32                         draw_count,
                              u32                         stride )
{
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );
	FT_FROM_HANDLE( buffer, ibuffer, VulkanBuffer );

	vkCmdDrawIndexedIndirect( cmd->command_buffer,
	                          buffer->buffer,
	                          offset,
	                          draw_count,
	                          stride );
}

static void
vk_cmd_bind_descriptor_set( const struct CommandBuffer* icmd,
                            u32                         first_set,
                            const struct DescriptorSet* iset,
                            const struct Pipeline*      ipipeline )
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
	    NULL );
}

void
vk_create_renderer_backend( const struct RendererBackendInfo* info,
                            struct RendererBackend**          p )
{
	destroy_renderer_backend_impl      = vk_destroy_renderer_backend;
	create_device_impl                 = vk_create_device;
	destroy_device_impl                = vk_destroy_device;
	create_queue_impl                  = vk_create_queue;
	destroy_queue_impl                 = vk_destroy_queue;
	queue_wait_idle_impl               = vk_queue_wait_idle;
	queue_submit_impl                  = vk_queue_submit;
	immediate_submit_impl              = vk_immediate_submit;
	queue_present_impl                 = vk_queue_present;
	create_semaphore_impl              = vk_create_semaphore;
	destroy_semaphore_impl             = vk_destroy_semaphore;
	create_fence_impl                  = vk_create_fence;
	destroy_fence_impl                 = vk_destroy_fence;
	wait_for_fences_impl               = vk_wait_for_fences;
	reset_fences_impl                  = vk_reset_fences;
	create_swapchain_impl              = vk_create_swapchain;
	resize_swapchain_impl              = vk_resize_swapchain;
	destroy_swapchain_impl             = vk_destroy_swapchain;
	create_command_pool_impl           = vk_create_command_pool;
	destroy_command_pool_impl          = vk_destroy_command_pool;
	create_command_buffers_impl        = vk_create_command_buffers;
	free_command_buffers_impl          = vk_free_command_buffers;
	destroy_command_buffers_impl       = vk_destroy_command_buffers;
	begin_command_buffer_impl          = vk_begin_command_buffer;
	end_command_buffer_impl            = vk_end_command_buffer;
	acquire_next_image_impl            = vk_acquire_next_image;
	create_shader_impl                 = vk_create_shader;
	destroy_shader_impl                = vk_destroy_shader;
	create_descriptor_set_layout_impl  = vk_create_descriptor_set_layout;
	destroy_descriptor_set_layout_impl = vk_destroy_descriptor_set_layout;
	create_compute_pipeline_impl       = vk_create_compute_pipeline;
	create_graphics_pipeline_impl      = vk_create_graphics_pipeline;
	destroy_pipeline_impl              = vk_destroy_pipeline;
	create_buffer_impl                 = vk_create_buffer;
	destroy_buffer_impl                = vk_destroy_buffer;
	map_memory_impl                    = vk_map_memory;
	unmap_memory_impl                  = vk_unmap_memory;
	create_sampler_impl                = vk_create_sampler;
	destroy_sampler_impl               = vk_destroy_sampler;
	create_image_impl                  = vk_create_image;
	destroy_image_impl                 = vk_destroy_image;
	create_descriptor_set_impl         = vk_create_descriptor_set;
	destroy_descriptor_set_impl        = vk_destroy_descriptor_set;
	update_descriptor_set_impl         = vk_update_descriptor_set;
	cmd_begin_render_pass_impl         = vk_cmd_begin_render_pass;
	cmd_end_render_pass_impl           = vk_cmd_end_render_pass;
	cmd_barrier_impl                   = vk_cmd_barrier;
	cmd_set_scissor_impl               = vk_cmd_set_scissor;
	cmd_set_viewport_impl              = vk_cmd_set_viewport;
	cmd_bind_pipeline_impl             = vk_cmd_bind_pipeline;
	cmd_draw_impl                      = vk_cmd_draw;
	cmd_draw_indexed_impl              = vk_cmd_draw_indexed;
	cmd_bind_vertex_buffer_impl        = vk_cmd_bind_vertex_buffer;
	cmd_bind_index_buffer_u16_impl     = vk_cmd_bind_index_buffer_u16;
	cmd_bind_index_buffer_u32_impl     = vk_cmd_bind_index_buffer_u32;
	cmd_copy_buffer_impl               = vk_cmd_copy_buffer;
	cmd_copy_buffer_to_image_impl      = vk_cmd_copy_buffer_to_image;
	cmd_bind_descriptor_set_impl       = vk_cmd_bind_descriptor_set;
	cmd_dispatch_impl                  = vk_cmd_dispatch;
	cmd_push_constants_impl            = vk_cmd_push_constants;
	cmd_clear_color_image_impl         = vk_cmd_clear_color_image;
	cmd_draw_indexed_indirect_impl     = vk_cmd_draw_indexed_indirect;

	FT_INIT_INTERNAL( backend, *p, VulkanRendererBackend );

	// TODO: provide posibility to set allocator from user code
	backend->vulkan_allocator = NULL;
	// TODO: same
	backend->api_version = VK_API_VERSION_1_2;

	volkInitialize();

	VkApplicationInfo app_info  = {};
	app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pNext              = NULL;
	app_info.pApplicationName   = "Fluent";
	app_info.applicationVersion = VK_MAKE_VERSION( 0, 0, 1 );
	app_info.pEngineName        = "Fluent-Engine";
	app_info.engineVersion      = VK_MAKE_VERSION( 0, 0, 1 );
	app_info.apiVersion         = backend->api_version;

	u32 instance_create_flags = 0;
	u32 extension_count       = 0;
	u32 layer_count           = 0;

	get_instance_extensions( info,
	                         &instance_create_flags,
	                         &extension_count,
	                         NULL );
	ALLOC_STACK_ARRAY( const char*, extensions, extension_count );
	get_instance_extensions( info,
	                         &instance_create_flags,
	                         &extension_count,
	                         extensions );

	get_instance_layers( &layer_count, NULL );
	ALLOC_STACK_ARRAY( const char*, layers, layer_count );
	get_instance_layers( &layer_count, layers );

	VkInstanceCreateInfo instance_create_info = {};
	instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_create_info.pNext = NULL;
	instance_create_info.pApplicationInfo        = &app_info;
	instance_create_info.enabledLayerCount       = layer_count;
	instance_create_info.ppEnabledLayerNames     = layers;
	instance_create_info.enabledExtensionCount   = extension_count;
	instance_create_info.ppEnabledExtensionNames = extensions;
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
	vkEnumeratePhysicalDevices( backend->instance, &device_count, NULL );
	FT_ASSERT( device_count != 0 );
	ALLOC_STACK_ARRAY( VkPhysicalDevice, physical_devices, device_count );
	vkEnumeratePhysicalDevices( backend->instance,
	                            &device_count,
	                            physical_devices );
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

#endif
