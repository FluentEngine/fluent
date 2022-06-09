#include "log/log.h"
#include "vulkan_pass_hasher.h"

// TODO: stack allocation instead of heap allocations
struct VulkanPassData
{
	VkAttachmentDescription* attachments;
	VkSubpassDescription*    subpasses;
	VkAttachmentReference**  color_references;
	VkAttachmentReference**  depth_references;
};

struct RenderPassMapItem
{
	struct VulkanPassData  data;
	VkRenderPassCreateInfo info;
	VkRenderPass           render_pass;
};

struct FramebufferMapItem
{
	VkImageView*            image_views;
	VkFramebufferCreateInfo info;
	VkFramebuffer           framebuffer;
};

static inline i32
compare_attachment_descriptions( const VkAttachmentDescription* a,
                                 const VkAttachmentDescription* b )
{
	if ( a->format != b->format )
	{
		return 1;
	}

	if ( a->samples != b->samples )
	{
		return 1;
	}

	if ( a->loadOp != b->loadOp )
	{
		return 1;
	}

	if ( a->initialLayout != b->initialLayout )
	{
		return 1;
	}

	if ( a->finalLayout != b->finalLayout )
	{
		return 1;
	}

	return 0;
}

static inline i32
compare_attachment_reference_array(
    u32                            count,
    const VkAttachmentDescription* attachments_a,
    const VkAttachmentReference*   a,
    const VkAttachmentDescription* attachments_b,
    const VkAttachmentReference*   b )
{
	for ( u32 i = 0; i < count; ++i )
	{
		const VkAttachmentReference* ref_a = &a[ i ];
		const VkAttachmentReference* ref_b = &b[ i ];

		if ( ref_a->layout != ref_b->layout )
		{
			return 1;
		}

		if ( compare_attachment_descriptions(
		         &attachments_a[ ref_a->attachment ],
		         &attachments_b[ ref_b->attachment ] ) != 0 )
		{
			return 1;
		}
	}

	return 0;
}

static int cnt = 0;

static inline i32
compare_pass_info( const void* a, const void* b, void* udata )
{
	FT_UNUSED( udata );

	const struct RenderPassMapItem* itema = a;
	const struct RenderPassMapItem* itemb = b;
	const VkRenderPassCreateInfo*   rpa   = &itema->info;
	const VkRenderPassCreateInfo*   rpb   = &itemb->info;

	if ( rpa->subpassCount != rpb->subpassCount )
	{
		return 1;
	}

	for ( u32 s = 0; s < rpa->subpassCount; ++s )
	{
		const VkSubpassDescription* sa = &rpa->pSubpasses[ s ];
		const VkSubpassDescription* sb = &rpb->pSubpasses[ s ];

		if ( sa->colorAttachmentCount != sb->colorAttachmentCount )
		{
			return 1;
		}

		if ( sa->pDepthStencilAttachment != sb->pDepthStencilAttachment &&
		     ( sa->pDepthStencilAttachment == NULL ||
		       sb->pDepthStencilAttachment == NULL ) )
		{
			return 1;
		}

		if ( compare_attachment_reference_array(
		         rpa->pSubpasses[ s ].colorAttachmentCount,
		         rpa->pAttachments,
		         rpa->pSubpasses[ s ].pColorAttachments,
		         rpb->pAttachments,
		         rpb->pSubpasses[ s ].pColorAttachments ) != 0 )
		{
			return 1;
		}

		if ( compare_attachment_reference_array(
		         rpa->pSubpasses[ s ].pDepthStencilAttachment != NULL,
		         rpa->pAttachments,
		         rpa->pSubpasses[ s ].pDepthStencilAttachment,
		         rpb->pAttachments,
		         rpb->pSubpasses[ s ].pDepthStencilAttachment ) != 0 )
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

	const struct FramebufferMapItem* itema = a;
	const struct FramebufferMapItem* itemb = b;
	const VkFramebufferCreateInfo*   fba   = &itema->info;
	const VkFramebufferCreateInfo*   fbb   = &itemb->info;

	if ( fba->attachmentCount != fbb->attachmentCount )
	{
		return 1;
	}

	for ( u32 i = 0; i < fba->attachmentCount; ++i )
	{
		if ( fba->pAttachments[ i ] != fbb->pAttachments[ i ] )
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

static inline void
free_render_pass_map_item( struct VulkanPassHasher*  hasher,
                           struct RenderPassMapItem* item )
{
	for ( u32 s = 0; s < item->info.subpassCount; ++s )
	{
		if ( item->data.color_references[ s ] )
		{
			free( item->data.color_references[ s ] );
		}

		if ( item->data.depth_references[ s ] )
		{
			free( item->data.depth_references[ s ] );
		}
	}
	free( item->data.color_references );
	free( item->data.depth_references );
	free( item->data.subpasses );
	free( item->data.attachments );

	vkDestroyRenderPass( hasher->device->logical_device,
	                     item->render_pass,
	                     hasher->device->vulkan_allocator );
}

static inline void
free_framebuffer_map_item( struct VulkanPassHasher*   hasher,
                           struct FramebufferMapItem* item )
{
	free( item->image_views );

	vkDestroyFramebuffer( hasher->device->logical_device,
	                      item->framebuffer,
	                      hasher->device->vulkan_allocator );
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
		free_framebuffer_map_item( hasher, item );
	}
	hashmap_free( hasher->framebuffers );

	iter = 0;

	while ( hashmap_iter( hasher->render_passes, &iter, &item ) )
	{
		free_render_pass_map_item( hasher, item );
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
		free_framebuffer_map_item( hasher, item );
	}
	hashmap_clear( hasher->framebuffers, 0 );
}

VkRenderPass
vk_pass_hasher_get_render_pass( struct VulkanPassHasher*      hasher,
                                const VkRenderPassCreateInfo* info )
{
	struct RenderPassMapItem* it = hashmap_get( hasher->render_passes,
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
		VK_ASSERT( vkCreateRenderPass( hasher->device->logical_device,
		                               info,
		                               hasher->device->vulkan_allocator,
		                               &render_pass ) );

		struct RenderPassMapItem item;
		memset( &item, 0, sizeof( struct RenderPassMapItem ) );
		item.info = *info;

		item.data.attachments =
		    calloc( info->attachmentCount, sizeof( VkAttachmentDescription ) );

		for ( u32 i = 0; i < info->attachmentCount; ++i )
		{
			item.data.attachments[ i ] = info->pAttachments[ i ];
		}

		item.data.subpasses =
		    calloc( info->subpassCount, sizeof( VkSubpassDescription ) );

		for ( u32 i = 0; i < info->subpassCount; ++i )
		{
			item.data.subpasses[ i ] = info->pSubpasses[ i ];
		}

		item.data.color_references =
		    calloc( info->subpassCount, sizeof( VkAttachmentReference* ) );
		item.data.depth_references =
		    calloc( info->subpassCount, sizeof( VkAttachmentReference* ) );

		for ( u32 s = 0; s < info->subpassCount; ++s )
		{
			item.data.color_references[ s ] =
			    calloc( info->pSubpasses[ s ].colorAttachmentCount,
			            sizeof( VkAttachmentReference ) );

			for ( u32 i = 0; i < info->pSubpasses[ s ].colorAttachmentCount;
			      ++i )
			{
				item.data.color_references[ s ][ i ] =
				    info->pSubpasses[ s ].pColorAttachments[ i ];
			}

			item.data.subpasses[ s ].pColorAttachments =
			    item.data.color_references[ s ];

			if ( info->pSubpasses[ s ].pDepthStencilAttachment != NULL )
			{
				item.data.depth_references[ s ] =
				    calloc( 1, sizeof( VkAttachmentReference ) );

				item.data.depth_references[ s ][ 0 ] =
				    *info->pSubpasses[ s ].pDepthStencilAttachment;
				item.data.subpasses[ s ].pDepthStencilAttachment =
				    &item.data.depth_references[ s ][ 0 ];
			}
		}

		item.info.pSubpasses   = item.data.subpasses;
		item.info.pAttachments = item.data.attachments;
		item.render_pass       = render_pass;

		hashmap_set( hasher->render_passes, &item );

		return render_pass;
	}
}

VkFramebuffer
vk_pass_hasher_get_framebuffer( struct VulkanPassHasher*       hasher,
                                const VkFramebufferCreateInfo* info )
{
	struct FramebufferMapItem* it =
	    hashmap_get( hasher->framebuffers,
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
		VK_ASSERT( vkCreateFramebuffer( hasher->device->logical_device,
		                                info,
		                                hasher->device->vulkan_allocator,
		                                &framebuffer ) );

		struct FramebufferMapItem item;
		memset( &item, 0, sizeof( struct FramebufferMapItem ) );
		item.info = *info;
		item.image_views =
		    calloc( info->attachmentCount, sizeof( VkImageView ) );
		item.info.pAttachments = item.image_views;
		item.framebuffer       = framebuffer;

		memcpy( item.image_views,
		        info->pAttachments,
		        sizeof( VkImageView ) * info->attachmentCount );

		hashmap_set( hasher->framebuffers, &item );

		return framebuffer;
	}
}
