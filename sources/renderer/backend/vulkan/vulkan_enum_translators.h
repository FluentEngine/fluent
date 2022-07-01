#pragma once

#include <volk/volk.h>
#include <tiny_image_format/tinyimageformat_apis.h>
#include <vk_mem_alloc/vk_mem_alloc.h>
#include "base/base.h"

FT_INLINE VkFormat
to_vk_format( enum ft_format format )
{
	return ( VkFormat ) TinyImageFormat_ToVkFormat(
	    ( TinyImageFormat ) format );
}

FT_INLINE enum ft_format
from_vk_format( VkFormat format )
{
	return ( enum ft_format ) TinyImageFormat_FromVkFormat(
	    ( TinyImageFormat_VkFormat ) format );
}

FT_INLINE VkSampleCountFlagBits
to_vk_sample_count( uint32_t sample_count )
{
	// TODO: assert?
	return ( VkSampleCountFlagBits ) sample_count;
}

FT_INLINE VkAttachmentLoadOp
to_vk_load_op( enum ft_attachment_load_op load_op )
{
	switch ( load_op )
	{
	case FT_ATTACHMENT_LOAD_OP_CLEAR: return VK_ATTACHMENT_LOAD_OP_CLEAR;
	case FT_ATTACHMENT_LOAD_OP_DONT_CARE:
		return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	case FT_ATTACHMENT_LOAD_OP_LOAD: return VK_ATTACHMENT_LOAD_OP_LOAD;
	default: FT_ASSERT( false ); return ( VkAttachmentLoadOp ) -1;
	}
}

FT_INLINE VkQueueFlagBits
to_vk_queue_type( enum ft_queue_type type )
{
	switch ( type )
	{
	case FT_QUEUE_TYPE_GRAPHICS: return VK_QUEUE_GRAPHICS_BIT;
	case FT_QUEUE_TYPE_COMPUTE: return VK_QUEUE_COMPUTE_BIT;
	case FT_QUEUE_TYPE_TRANSFER: return VK_QUEUE_TRANSFER_BIT;
	default: FT_ASSERT( false ); return ( VkQueueFlagBits ) -1;
	}
}

FT_INLINE VkVertexInputRate
to_vk_vertex_input_rate( enum ft_vertex_input_rate input_rate )
{
	switch ( input_rate )
	{
	case FT_VERTEX_INPUT_RATE_VERTEX: return VK_VERTEX_INPUT_RATE_VERTEX;
	case FT_VERTEX_INPUT_RATE_INSTANCE: return VK_VERTEX_INPUT_RATE_INSTANCE;
	default: FT_ASSERT( false ); return ( VkVertexInputRate ) -1;
	}
}

FT_INLINE VkIndexType
to_vk_index_type( enum ft_index_type index_type )
{
	switch ( index_type )
	{
	case FT_INDEX_TYPE_U16: return VK_INDEX_TYPE_UINT16;
	case FT_INDEX_TYPE_U32: return VK_INDEX_TYPE_UINT32;
	default: FT_ASSERT( false ); return ( VkIndexType ) -1;
	}
}

FT_INLINE VkCullModeFlagBits
to_vk_cull_mode( enum ft_cull_mode cull_mode )
{
	switch ( cull_mode )
	{
	case FT_CULL_MODE_BACK: return VK_CULL_MODE_BACK_BIT;
	case FT_CULL_MODE_FRONT: return VK_CULL_MODE_FRONT_BIT;
	case FT_CULL_MODE_NONE: return VK_CULL_MODE_NONE;
	default: FT_ASSERT( false ); return ( VkCullModeFlagBits ) -1;
	}
}

FT_INLINE VkFrontFace
to_vk_front_face( enum ft_front_face front_face )
{
	switch ( front_face )
	{
	case FT_FRONT_FACE_CLOCKWISE: return VK_FRONT_FACE_CLOCKWISE;
	case FT_FRONT_FACE_COUNTER_CLOCKWISE:
		return VK_FRONT_FACE_COUNTER_CLOCKWISE;
	default: FT_ASSERT( false ); return ( VkFrontFace ) -1;
	}
}

FT_INLINE VkCompareOp
to_vk_compare_op( enum ft_compare_op op )
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
	default: FT_ASSERT( false ); return ( VkCompareOp ) -1;
	}
}

FT_INLINE VkShaderStageFlagBits
to_vk_shader_stage( enum ft_shader_stage shader_stage )
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
	default: FT_ASSERT( false ); return ( VkShaderStageFlagBits ) -1;
	}
}

FT_INLINE VkPipelineBindPoint
to_vk_pipeline_bind_point( enum ft_pipeline_type type )
{
	switch ( type )
	{
	case FT_PIPELINE_TYPE_COMPUTE: return VK_PIPELINE_BIND_POINT_COMPUTE;
	case FT_PIPELINE_TYPE_GRAPHICS: return VK_PIPELINE_BIND_POINT_GRAPHICS;
	default: FT_ASSERT( false ); return ( VkPipelineBindPoint ) -1;
	}
}

FT_INLINE VkDescriptorType
to_vk_descriptor_type( enum ft_descriptor_type descriptor_type )
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
	default: FT_ASSERT( false ); return ( VkDescriptorType ) -1;
	}
}

FT_INLINE VkFilter
to_vk_filter( enum ft_filter filter )
{
	switch ( filter )
	{
	case FT_FILTER_LINEAR: return VK_FILTER_LINEAR;
	case FT_FILTER_NEAREST: return VK_FILTER_NEAREST;
	default: FT_ASSERT( false ); return ( VkFilter ) -1;
	}
}

FT_INLINE VkSamplerMipmapMode
to_vk_sampler_mipmap_mode( enum ft_sampler_mipmap_mode mode )
{
	switch ( mode )
	{
	case FT_SAMPLER_MIPMAP_MODE_NEAREST: return VK_SAMPLER_MIPMAP_MODE_NEAREST;
	case FT_SAMPLER_MIPMAP_MODE_LINEAR: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	default: FT_ASSERT( false ); return ( VkSamplerMipmapMode ) -1;
	}
}

FT_INLINE VkSamplerAddressMode
to_vk_sampler_address_mode( enum ft_sampler_address_mode mode )
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
	default: FT_ASSERT( false ); return ( VkSamplerAddressMode ) -1;
	}
}

FT_INLINE VkPolygonMode
to_vk_polygon_mode( enum ft_polygon_mode mode )
{
	switch ( mode )
	{
	case FT_POLYGON_MODE_FILL: return VK_POLYGON_MODE_FILL;
	case FT_POLYGON_MODE_LINE: return VK_POLYGON_MODE_LINE;
	default: FT_ASSERT( false ); return ( VkPolygonMode ) -1;
	}
}

FT_INLINE VkPrimitiveTopology
to_vk_primitive_topology( enum ft_primitive_topology topology )
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
	default: FT_ASSERT( false ); return ( VkPrimitiveTopology ) -1;
	}
}

FT_INLINE VkBlendFactor
to_vk_blend_factor( enum ft_blend_factor factor )
{
	switch ( factor )
	{
	case FT_BLEND_FACTOR_ZERO: return VK_BLEND_FACTOR_ZERO;
	case FT_BLEND_FACTOR_ONE: return VK_BLEND_FACTOR_ONE;
	case FT_BLEND_FACTOR_SRC_COLOR: return VK_BLEND_FACTOR_SRC_COLOR;
	case FT_BLEND_FACTOR_ONE_MINUS_SRC_COLOR:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
	case FT_BLEND_FACTOR_DST_COLOR: return VK_BLEND_FACTOR_DST_COLOR;
	case FT_BLEND_FACTOR_ONE_MINUS_DST_COLOR:
		return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
	case FT_BLEND_FACTOR_SRC_ALPHA: return VK_BLEND_FACTOR_SRC_ALPHA;
	case FT_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	case FT_BLEND_FACTOR_DST_ALPHA: return VK_BLEND_FACTOR_DST_ALPHA;
	case FT_BLEND_FACTOR_ONE_MINUS_DST_ALPHA:
		return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
	case FT_BLEND_FACTOR_SRC_ALPHA_SATURATE:
		return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
	default: FT_ASSERT( false ); return ( VkBlendFactor ) -1;
	}
}

FT_INLINE VkBlendOp
to_vk_blend_op( enum ft_blend_op op )
{
	switch ( op )
	{
	case FT_BLEND_OP_ADD: return VK_BLEND_OP_ADD;
	case FT_BLEND_OP_SUBTRACT: return VK_BLEND_OP_SUBTRACT;
	case FT_BLEND_OP_REVERSE_SUBTRACT: return VK_BLEND_OP_REVERSE_SUBTRACT;
	case FT_BLEND_OP_MIN: return VK_BLEND_OP_MIN;
	case FT_BLEND_OP_MAX: return VK_BLEND_OP_MAX;
	default: FT_ASSERT( false ); return ( VkBlendOp ) -1;
	}
}

FT_INLINE VkAccessFlags
determine_access_flags( enum ft_resource_state resource_state )
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

FT_INLINE VkPipelineStageFlags
determine_pipeline_stage_flags( VkAccessFlags      access_flags,
                                enum ft_queue_type queue_type )
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

FT_INLINE VkImageLayout
determine_image_layout( enum ft_resource_state resource_state )
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

FT_INLINE VmaMemoryUsage
determine_vma_memory_usage( enum ft_memory_usage usage )
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
	FT_ASSERT( false );
	return ( VmaMemoryUsage ) -1;
}

FT_INLINE VkBufferUsageFlags
determine_vk_buffer_usage( enum ft_descriptor_type descriptor_type,
                           enum ft_memory_usage    memory_usage )
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

FT_INLINE VkImageUsageFlags
determine_vk_image_usage( enum ft_descriptor_type descriptor_type )
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
