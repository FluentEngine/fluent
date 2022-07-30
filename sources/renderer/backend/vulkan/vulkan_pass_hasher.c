#ifdef VULKAN_BACKEND

#include "../renderer_private.h"
#include "vulkan_enum_translators.h"
#include "vulkan_pass_hasher.h"

struct vulkan_pass_hasher
{
	const struct vk_device* device;
	struct hashmap*         render_passes;
	struct hashmap*         framebuffers;
};

static struct vulkan_pass_hasher hasher;

struct render_pass_map_item
{
	struct ft_render_pass_begin_info info;
	VkRenderPass                     render_pass;
};

struct framebuffer_map_item
{
	struct ft_render_pass_begin_info info;
	VkFramebuffer                    framebuffer;
};

FT_INLINE bool
has_depth_stencil( const struct ft_render_pass_begin_info* info )
{
	return info->depth_attachment.image != NULL;
}

FT_INLINE int32_t
compare_attachments( const struct ft_attachment_info* a,
                     const struct ft_attachment_info* b )
{
	const struct ft_image* ia = a->image;
	const struct ft_image* ib = b->image;

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

	return 0;
}

static int
compare_pass_info( const void* a, const void* b, void* udata )
{
	FT_UNUSED( udata );

	const struct ft_render_pass_begin_info* rpa = a;
	const struct ft_render_pass_begin_info* rpb = b;
	if ( rpa->color_attachment_count != rpb->color_attachment_count )
		return 0;

	if ( has_depth_stencil( rpa ) != has_depth_stencil( rpb ) )
	{
		return 1;
	}

	for ( uint32_t i = 0; i < rpa->color_attachment_count; ++i )
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

static uint64_t
hash_pass_info( const void* item, uint64_t seed0, uint64_t seed1 )
{
	FT_UNUSED( item );
	FT_UNUSED( seed0 );
	FT_UNUSED( seed1 );
	// TODO: hash function
	return 0;
}

static int
compare_framebuffer_info( const void* a, const void* b, void* udata )
{
	FT_UNUSED( udata );

	const struct ft_render_pass_begin_info* rpa = a;
	const struct ft_render_pass_begin_info* rpb = b;

	if ( rpa->color_attachment_count != rpb->color_attachment_count )
	{
		return 1;
	}

	for ( uint32_t i = 0; i < rpa->color_attachment_count; ++i )
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

static uint64_t
hash_framebuffer_info( const void* item, uint64_t seed0, uint64_t seed1 )
{
	FT_UNUSED( item );
	FT_UNUSED( seed0 );
	FT_UNUSED( seed1 );
	// TODO: hash function
	return 0;
}

void
vk_create_render_pass( const struct vk_device*                 device,
                       const struct ft_render_pass_begin_info* info,
                       VkRenderPass*                           p )
{
	uint32_t attachments_count = info->color_attachment_count;

	VkAttachmentDescription attachments[ FT_MAX_ATTACHMENTS_COUNT + 2 ];
	VkAttachmentReference   color_references[ FT_MAX_ATTACHMENTS_COUNT ];
	VkAttachmentReference   depth_reference = { 0 };

	for ( uint32_t i = 0; i < info->color_attachment_count; ++i )
	{
		const struct ft_attachment_info* att = &info->color_attachments[ i ];

		attachments[ i ] = ( VkAttachmentDescription ) {
		    .flags          = 0,
		    .format         = to_vk_format( att->image->format ),
		    .samples        = to_vk_sample_count( att->image->sample_count ),
		    .loadOp         = to_vk_load_op( att->load_op ),
		    .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
		    .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		    .initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		    .finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		};

		color_references[ i ] = ( VkAttachmentReference ) {
		    .attachment = i,
		    .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		};
	}

	if ( has_depth_stencil( info ) )
	{
		uint32_t                         i   = attachments_count;
		const struct ft_attachment_info* att = &info->depth_attachment;

		attachments[ i ] = ( VkAttachmentDescription ) {
		    .flags          = 0,
		    .format         = to_vk_format( att->image->format ),
		    .samples        = to_vk_sample_count( att->image->sample_count ),
		    .loadOp         = to_vk_load_op( att->load_op ),
		    .storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		    .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		    .initialLayout  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		    .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		};

		depth_reference = ( VkAttachmentReference ) {
		    .attachment = i,
		    .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
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

FT_INLINE void
vk_create_framebuffer( const struct vk_device*                 device,
                       const struct ft_render_pass_begin_info* info,
                       VkRenderPass                            render_pass,
                       VkFramebuffer*                          p )
{
	uint32_t attachment_count = info->color_attachment_count;

	VkImageView image_views[ FT_MAX_ATTACHMENTS_COUNT + 2 ];
	for ( uint32_t i = 0; i < attachment_count; ++i )
	{
		FT_FROM_HANDLE( image, info->color_attachments[ i ].image, vk_image );
		image_views[ i ] = image->sampled_view;
	}

	if ( info->depth_attachment.image != NULL )
	{
		FT_FROM_HANDLE( image, info->depth_attachment.image, vk_image );
		image_views[ attachment_count++ ] = image->sampled_view;
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
vk_pass_hasher_init( const struct vk_device* device )
{
	hasher.device = device;

	hasher.render_passes = hashmap_new( sizeof( struct render_pass_map_item ),
	                                    0,
	                                    0,
	                                    0,
	                                    hash_pass_info,
	                                    compare_pass_info,
	                                    NULL,
	                                    NULL );

	hasher.framebuffers = hashmap_new( sizeof( struct framebuffer_map_item ),
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
		struct framebuffer_map_item* it = item;
		vkDestroyFramebuffer( hasher.device->logical_device,
		                      it->framebuffer,
		                      hasher.device->vulkan_allocator );
	}
	hashmap_free( hasher.framebuffers );

	iter = 0;

	while ( hashmap_iter( hasher.render_passes, &iter, &item ) )
	{
		struct render_pass_map_item* it = item;
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
		struct framebuffer_map_item* info = item;
		vkDestroyFramebuffer( hasher.device->logical_device,
		                      info->framebuffer,
		                      hasher.device->vulkan_allocator );
	}
	hashmap_clear( hasher.framebuffers, 0 );
}

VkRenderPass
vk_pass_hasher_get_render_pass( const struct ft_render_pass_begin_info* info )
{
	struct render_pass_map_item* it =
	    hashmap_get( hasher.render_passes,
	                 &( struct render_pass_map_item ) {
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
		             &( struct render_pass_map_item ) {
		                 .info        = *info,
		                 .render_pass = render_pass,
		             } );

		return render_pass;
	}
}

VkFramebuffer
vk_pass_hasher_get_framebuffer( VkRenderPass render_pass,
                                const struct ft_render_pass_begin_info* info )
{
	struct framebuffer_map_item* it =
	    hashmap_get( hasher.framebuffers,
	                 &( struct framebuffer_map_item ) {
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
		             &( struct framebuffer_map_item ) {
		                 .info        = *info,
		                 .framebuffer = framebuffer,
		             } );

		return framebuffer;
	}
}

#endif
