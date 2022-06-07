#include "log/log.h"
#include "vulkan_pass_hasher.h"

static inline void
vk_create_render_pass( const struct VulkanDevice*   device,
                       const struct VulkanPassInfo* info,
                       VkRenderPass*                p )
{
	u32 attachments_count = info->color_attachment_count;

	VkAttachmentDescription
	                      attachment_descriptions[ MAX_ATTACHMENTS_COUNT + 2 ];
	VkAttachmentReference color_attachment_references[ MAX_ATTACHMENTS_COUNT ];
	VkAttachmentReference depth_attachment_reference = { 0 };

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
		    determine_image_layout( info->color_image_initial_states[ i ] );
		attachment_descriptions[ i ].finalLayout =
		    determine_image_layout( info->color_image_final_states[ i ] );

		color_attachment_references[ i ].attachment = i;
		color_attachment_references[ i ].layout =
		    attachment_descriptions[ i ].finalLayout;
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
		    determine_image_layout( info->depth_stencil_initial_state );
		attachment_descriptions[ i ].finalLayout =
		    determine_image_layout( info->depth_stencil_final_state );

		depth_attachment_reference.attachment = i;
		depth_attachment_reference.layout =
		    attachment_descriptions[ i ].finalLayout;

		attachments_count++;
	}

	VkSubpassDescription subpass_description = { 0 };
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

	VkRenderPassCreateInfo render_pass_create_info = { 0 };
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

static inline void
vk_create_framebuffer( const struct VulkanDevice*   device,
                       const struct VulkanPassInfo* info,
                       VkRenderPass                 render_pass,
                       VkFramebuffer*               p )
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

	VkFramebufferCreateInfo framebuffer_create_info = { 0 };
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

struct RenderPassMapItem
{
	struct VulkanPassInfo info;
	VkRenderPass          value;
};

struct FramebufferMapItem
{
	struct VulkanPassInfo info;
	VkFramebuffer         value;
};

static inline b32
compare_pass_info( const void* a, const void* b, void* udata )
{
	FT_UNUSED( udata );

	const struct VulkanPassInfo* rpa = a;
	const struct VulkanPassInfo* rpb = b;
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

		if ( rpa->color_image_initial_states[ i ] !=
		     rpb->color_image_initial_states[ i ] )
		{
			return 1;
		}

		if ( rpa->color_image_final_states[ i ] !=
		     rpb->color_image_final_states[ i ] )
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

		if ( rpa->depth_stencil_initial_state !=
		     rpb->depth_stencil_initial_state )
		{
			return 1;
		}

		if ( rpa->depth_stencil_final_state != rpb->depth_stencil_final_state )
		{
			return 1;
		}
	}

	return 0;
}

static inline u64
hash_pass_info( const void* item, u64 seed0, u64 seed1 )
{
	FT_UNUSED( item );
	FT_UNUSED( seed0 );
	FT_UNUSED( seed1 );
	// TODO: hash function
	return 0;
}

static inline b32
compare_framebuffer_info( const void* a, const void* b, void* udata )
{
	FT_UNUSED( udata );

	const struct VulkanPassInfo* rpa = a;
	const struct VulkanPassInfo* rpb = b;

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

static inline u64
hash_framebuffer_info( const void* item, u64 seed0, u64 seed1 )
{
	FT_UNUSED( item );
	FT_UNUSED( seed0 );
	FT_UNUSED( seed1 );
	// TODO: hash function
	return 0;
}

void
vk_pass_hasher_init( struct VulkanPassHasher*   hasher,
                     const struct VulkanDevice* device )
{
	hasher->device = device;

	hasher->render_passes = hashmap_new( sizeof( struct RenderPassMapItem ),
	                                     0,
	                                     0,
	                                     0,
	                                     hash_pass_info,
	                                     compare_pass_info,
	                                     NULL,
	                                     NULL );

	hasher->framebuffers = hashmap_new( sizeof( struct FramebufferMapItem ),
	                                    0,
	                                    0,
	                                    0,
	                                    hash_framebuffer_info,
	                                    compare_framebuffer_info,
	                                    NULL,
	                                    NULL );
}

void
vk_pass_hasher_shutdown( struct VulkanPassHasher* hasher )
{
	u64   iter = 0;
	void* item;

	while ( hashmap_iter( hasher->framebuffers, &iter, &item ) )
	{
		struct FramebufferMapItem* fb = item;
		vkDestroyFramebuffer( hasher->device->logical_device,
		                      fb->value,
		                      hasher->device->vulkan_allocator );
	}
	hashmap_free( hasher->framebuffers );

	iter = 0;

	while ( hashmap_iter( hasher->render_passes, &iter, &item ) )
	{
		struct RenderPassMapItem* pass = item;
		vkDestroyRenderPass( hasher->device->logical_device,
		                     pass->value,
		                     hasher->device->vulkan_allocator );
	}

	hashmap_free( hasher->render_passes );
}

void
vk_pass_hasher_framebuffers_clear( struct VulkanPassHasher* hasher )
{
	u64   iter = 0;
	void* item;
	while ( hashmap_iter( hasher->framebuffers, &iter, &item ) )
	{
		struct FramebufferMapItem* fb = item;
		vkDestroyFramebuffer( hasher->device->logical_device,
		                      fb->value,
		                      hasher->device->vulkan_allocator );
	}
	hashmap_clear( hasher->framebuffers, 0 );
}

VkRenderPass
vk_pass_hasher_get_render_pass( struct VulkanPassHasher*     hasher,
                                const struct VulkanPassInfo* info )
{
	struct RenderPassMapItem* it =
	    hashmap_get( hasher->render_passes,
	                 &( struct RenderPassMapItem ) { .info = *info } );

	if ( it != NULL )
	{
		return it->value;
	}
	else
	{
		VkRenderPass render_pass;
		vk_create_render_pass( hasher->device, info, &render_pass );
		hashmap_set( hasher->render_passes,
		             &( struct RenderPassMapItem ) { .info  = *info,
		                                             .value = render_pass } );
		return render_pass;
	}
}

VkFramebuffer
vk_pass_hasher_get_framebuffer( struct VulkanPassHasher*     hasher,
                                VkRenderPass                 render_pass,
                                const struct VulkanPassInfo* info )
{
	FT_FROM_HANDLE( device, info->device, VulkanDevice );

	struct FramebufferMapItem* it =
	    hashmap_get( hasher->framebuffers,
	                 &( struct FramebufferMapItem ) { .info = *info } );

	if ( it != NULL )
	{
		return it->value;
	}
	else
	{
		VkFramebuffer framebuffer;
		vk_create_framebuffer( device, info, render_pass, &framebuffer );
		hashmap_set( hasher->framebuffers,
		             &( struct FramebufferMapItem ) { .info  = *info,
		                                              .value = framebuffer } );

		return framebuffer;
	}
}
