#ifdef VULKAN_BACKEND

#include "log/log.h"
#include "../renderer_private.h"
#include "vulkan_pass_hasher.h"

struct VulkanPassHasher
{
	const struct VulkanDevice* device;
	struct hashmap*            render_passes;
	struct hashmap*            framebuffers;
};

static struct VulkanPassHasher hasher;

struct RenderPassMapItem
{
	struct RenderPassBeginInfo info;
	VkRenderPass               render_pass;
};

struct FramebufferMapItem
{
	struct RenderPassBeginInfo info;
	VkFramebuffer              framebuffer;
};

static inline b32 has_depth_stencil(const struct RenderPassBeginInfo* info)
{
	return info->depth_attachment.image != NULL;
}

static inline i32
compare_attachments( const struct AttachmentInfo* a,
                     const struct AttachmentInfo* b )
{
	const struct Image* ia = a->image;
	const struct Image* ib = b->image;

	if ( ia->format != ib->format )
	{
		return 1;
	}

	if ( ia->sample_count != ib->sample_count )
	{
		return 1;
	}

	if ( a->load_op != b->load_op )
	{
		return 1;
	}

	if ( a->state != b->state )
	{
		return 1;
	}

	return 0;
}

static b32
compare_pass_info( const void* a, const void* b, void* udata )
{
	FT_UNUSED( udata );

	const struct RenderPassBeginInfo* rpa = a;
	const struct RenderPassBeginInfo* rpb = b;
	if ( rpa->color_attachment_count != rpb->color_attachment_count )
		return 0;

	if ( has_depth_stencil( rpa ) != has_depth_stencil( rpb ) )
	{
		return 1;
	}

	for ( u32 i = 0; i < rpa->color_attachment_count; ++i )
	{
		if ( compare_attachments( &rpa->color_attachments[ i ],
		                          &rpb->color_attachments[ i ] ) )
		{
			return 1;
		}
	}

	if ( has_depth_stencil( rpa ) )
	{
		if ( compare_attachments( &rpa->depth_attachment,
		                          &rpb->depth_attachment ) )
		{
			return 1;
		}
	}

	return 0;
}

static u64
hash_pass_info( const void* item, u64 seed0, u64 seed1 )
{
	FT_UNUSED( item );
	FT_UNUSED( seed0 );
	FT_UNUSED( seed1 );
	// TODO: hash function
	return 0;
}

static b32
compare_framebuffer_info( const void* a, const void* b, void* udata )
{
	FT_UNUSED( udata );

	const struct RenderPassBeginInfo* rpa = a;
	const struct RenderPassBeginInfo* rpb = b;

	if ( rpa->color_attachment_count != rpb->color_attachment_count )
	{
		return 1;
	}

	for ( u32 i = 0; i < rpa->color_attachment_count; ++i )
	{
		if ( rpa->color_attachments[ i ].image !=
		     rpb->color_attachments[ i ].image )
		{
			return 1;
		}
	}

	if ( has_depth_stencil( rpa ) != has_depth_stencil( rpb ) )
	{
		return 1;
	}

	if ( has_depth_stencil( rpa ) )
	{
		if ( rpa->depth_attachment.image != rpb->depth_attachment.image )
		{
			return 1;
		}
	}

	return 0;
}

static u64
hash_framebuffer_info( const void* item, u64 seed0, u64 seed1 )
{
	FT_UNUSED( item );
	FT_UNUSED( seed0 );
	FT_UNUSED( seed1 );
	// TODO: hash function
	return 0;
}

void
vk_create_render_pass( const struct VulkanDevice*        device,
                       const struct RenderPassBeginInfo* info,
                       VkRenderPass*                     p )
{
	u32 attachments_count = info->color_attachment_count;
	
	VkAttachmentDescription attachments[ MAX_ATTACHMENTS_COUNT + 2 ];
	VkAttachmentReference   color_references[ MAX_ATTACHMENTS_COUNT ];
	VkAttachmentReference   depth_reference = { 0 };

	for ( u32 i = 0; i < info->color_attachment_count; ++i )
	{
		const struct AttachmentInfo* att = &info->color_attachments[ i ];

		attachments[ i ] = ( VkAttachmentDescription ) {
			.flags          = 0,
			.format         = to_vk_format( att->image->format ),
			.samples        = to_vk_sample_count( att->image->sample_count ),
			.loadOp         = to_vk_load_op( att->load_op ),
			.storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout  = determine_image_layout( att->state ),
			.finalLayout    = determine_image_layout( att->state ),
		};

		color_references[ i ] = ( VkAttachmentReference ) {
			.attachment = i,
			.layout     = determine_image_layout( att->state ),
		};
	}

	if ( has_depth_stencil( info ) )
	{
		u32                          i   = attachments_count;
		const struct AttachmentInfo* att = &info->depth_attachment;

		attachments[ i ] = ( VkAttachmentDescription ) {
			.flags          = 0,
			.format         = to_vk_format( att->image->format ),
			.samples        = to_vk_sample_count( att->image->sample_count ),
			.loadOp         = to_vk_load_op( att->load_op ),
			.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout  = determine_image_layout( att->state ),
			.finalLayout    = determine_image_layout( att->state ),
		};

		depth_reference = ( VkAttachmentReference ) {
			.attachment = i,
			.layout     = determine_image_layout( att->state ),
		};

		attachments_count++;
	}

	VkSubpassDescription subpass = {
		.flags                = 0,
		.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.pInputAttachments    = NULL,
		.colorAttachmentCount = info->color_attachment_count,
		.pColorAttachments    = color_references,
		.pDepthStencilAttachment =
		    has_depth_stencil( info ) ? &depth_reference : NULL,
		.pResolveAttachments     = NULL,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments    = NULL,
	};

	VkRenderPassCreateInfo render_pass_create_info = {
		.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext           = NULL,
		.flags           = 0,
		.attachmentCount = attachments_count,
		.pAttachments    = attachments,
		.subpassCount    = 1,
		.pSubpasses      = &subpass,
		.dependencyCount = 0,
		.pDependencies   = NULL,
	};

	VK_ASSERT( vkCreateRenderPass( device->logical_device,
	                               &render_pass_create_info,
	                               device->vulkan_allocator,
	                               p ) );
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
		FT_FROM_HANDLE( image,
		                info->color_attachments[ i ].image,
		                VulkanImage );
		image_views[ i ] = image->image_view;
	}

	if ( info->depth_attachment.image != NULL )
	{
		FT_FROM_HANDLE( image, info->depth_attachment.image, VulkanImage );
		image_views[ attachment_count++ ] = image->image_view;
	}

	VkFramebufferCreateInfo framebuffer_create_info = {
		.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext           = NULL,
		.flags           = 0,
		.renderPass      = render_pass,
		.attachmentCount = attachment_count,
		.pAttachments    = image_views,
		.width           = info->width,
		.height          = info->height,
		.layers          = 1,
	};

	VK_ASSERT( vkCreateFramebuffer( device->logical_device,
	                                &framebuffer_create_info,
	                                device->vulkan_allocator,
	                                p ) );
}

void
vk_pass_hasher_init( const struct VulkanDevice* device )
{
	hasher.device = device;

	hasher.render_passes = hashmap_new( sizeof( struct RenderPassMapItem ),
	                                    0,
	                                    0,
	                                    0,
	                                    hash_pass_info,
	                                    compare_pass_info,
	                                    NULL,
	                                    NULL );

	hasher.framebuffers = hashmap_new( sizeof( struct FramebufferMapItem ),
	                                   0,
	                                   0,
	                                   0,
	                                   hash_framebuffer_info,
	                                   compare_framebuffer_info,
	                                   NULL,
	                                   NULL );
}

void
vk_pass_hasher_shutdown()
{
	size_t iter = 0;
	void*  item;

	while ( hashmap_iter( hasher.framebuffers, &iter, &item ) )
	{
		struct FramebufferMapItem* it = item;
		vkDestroyFramebuffer( hasher.device->logical_device,
		                      it->framebuffer,
		                      hasher.device->vulkan_allocator );
	}
	hashmap_free( hasher.framebuffers );

	iter = 0;

	while ( hashmap_iter( hasher.render_passes, &iter, &item ) )
	{
		struct RenderPassMapItem* it = item;
		vkDestroyRenderPass( hasher.device->logical_device,
		                     it->render_pass,
		                     hasher.device->vulkan_allocator );
	}

	hashmap_free( hasher.render_passes );
}

void
vk_pass_hasher_framebuffers_clear()
{
	size_t iter = 0;
	void*  item;
	while ( hashmap_iter( hasher.framebuffers, &iter, &item ) )
	{
		struct FramebufferMapItem* info = item;
		vkDestroyFramebuffer( hasher.device->logical_device,
		                      info->framebuffer,
		                      hasher.device->vulkan_allocator );
	}
	hashmap_clear( hasher.framebuffers, 0 );
}

VkRenderPass
vk_pass_hasher_get_render_pass( const struct RenderPassBeginInfo* info )
{
	struct RenderPassMapItem* it = hashmap_get( hasher.render_passes,
	                                            &( struct RenderPassMapItem ) {
	                                                .info = *info,
	                                            } );

	if ( it != NULL )
	{
		return it->render_pass;
	}
	else
	{
		VkRenderPass render_pass;
		vk_create_render_pass( hasher.device, info, &render_pass );

		hashmap_set( hasher.render_passes,
		             &( struct RenderPassMapItem ) {
		                 .info        = *info,
		                 .render_pass = render_pass,
		             } );

		return render_pass;
	}
}

VkFramebuffer
vk_pass_hasher_get_framebuffer( VkRenderPass                      render_pass,
                                const struct RenderPassBeginInfo* info )
{
	struct FramebufferMapItem* it =
	    hashmap_get( hasher.framebuffers,
	                 &( struct FramebufferMapItem ) {
	                     .info = *info,
	                 } );

	if ( it != NULL )
	{
		return it->framebuffer;
	}
	else
	{
		VkFramebuffer framebuffer;
		vk_create_framebuffer( hasher.device, info, render_pass, &framebuffer );

		hashmap_set( hasher.framebuffers,
		             &( struct FramebufferMapItem ) {
		                 .info        = *info,
		                 .framebuffer = framebuffer,
		             } );

		return framebuffer;
	}
}

#endif
