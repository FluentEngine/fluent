#include "log/log.h"
#include "vulkan_pass_hasher.h"

struct RenderPassMapItem
{
	struct VulkanRenderPassInfo info;
	VkRenderPass                render_pass;
};

struct FramebufferMapItem
{
	struct VulkanFramebufferInfo info;
	VkFramebuffer                framebuffer;
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

	const struct RenderPassMapItem* rpa = a;
	const struct RenderPassMapItem* rpb = b;

	if ( rpa->info.subpass_count != rpb->info.subpass_count )
	{
		return 1;
	}

	u32                             subpass_count = rpa->info.subpass_count;
	const struct VulkanSubpassInfo* sa            = rpa->info.subpasses;
	const struct VulkanSubpassInfo* sb            = rpb->info.subpasses;

	for ( u32 s = 0; s < subpass_count; ++s )
	{
		if ( sa[ s ].color_attachment_count != sb[ s ].color_attachment_count )
		{
			return 1;
		}

		if ( sa[ s ].has_depth_stencil != sb[ s ].has_depth_stencil )
		{
			return 1;
		}

		if ( compare_attachment_reference_array(
		         sa[ s ].color_attachment_count,
		         rpa->info.attachments,
		         sa[ s ].color_attachment_references,
		         rpb->info.attachments,
		         sb[ s ].color_attachment_references ) != 0 )
		{
			return 1;
		}

		if ( compare_attachment_reference_array(
		         sa[ s ].has_depth_stencil,
		         rpa->info.attachments,
		         &sa[ s ].depth_attachment_reference,
		         rpb->info.attachments,
		         &sb[ s ].depth_attachment_reference ) != 0 )
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

	const struct FramebufferMapItem* fba = a;
	const struct FramebufferMapItem* fbb = b;

	if ( fba->info.attachment_count != fbb->info.attachment_count )
	{
		return 1;
	}

	for ( u32 i = 0; i < fba->info.attachment_count; ++i )
	{
		if ( fba->info.attachments[ i ] != fbb->info.attachments[ i ] )
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
		struct FramebufferMapItem* it = item;
		vkDestroyFramebuffer( hasher->device->logical_device,
		                      it->framebuffer,
		                      hasher->device->vulkan_allocator );
	}
	hashmap_free( hasher->framebuffers );

	iter = 0;

	while ( hashmap_iter( hasher->render_passes, &iter, &item ) )
	{
		struct RenderPassMapItem* it = item;
		vkDestroyRenderPass( hasher->device->logical_device,
		                     it->render_pass,
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
		struct FramebufferMapItem* info = item;
		vkDestroyFramebuffer( hasher->device->logical_device,
		                      info->framebuffer,
		                      hasher->device->vulkan_allocator );
	}
	hashmap_clear( hasher->framebuffers, 0 );
}

VkRenderPass
vk_pass_hasher_get_render_pass( struct VulkanPassHasher*           hasher,
                                const struct VulkanRenderPassInfo* info )
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
		VkSubpassDescription subpasses[ 1 ];
		memset( subpasses,
		        0,
		        sizeof( VkSubpassDescription ) * info->subpass_count );

		for ( u32 i = 0; i < info->subpass_count; ++i )
		{
			subpasses[ i ] = ( VkSubpassDescription ) {
				.flags             = 0,
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				.colorAttachmentCount =
				    info->subpasses[ i ].color_attachment_count,
				.pColorAttachments =
				    info->subpasses[ i ].color_attachment_references,
				.pDepthStencilAttachment =
				    info->subpasses[ i ].has_depth_stencil
				        ? &info->subpasses[ i ].depth_attachment_reference
				        : NULL,
			};
		}

		VkRenderPassCreateInfo create_info = {
			.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.pNext           = NULL,
			.flags           = 0,
			.dependencyCount = 0,
			.pDependencies   = NULL,
			.attachmentCount = info->attachment_count,
			.pAttachments    = info->attachments,
			.subpassCount    = info->subpass_count,
			.pSubpasses      = subpasses,
		};

		VkRenderPass render_pass;
		VK_ASSERT( vkCreateRenderPass( hasher->device->logical_device,
		                               &create_info,
		                               hasher->device->vulkan_allocator,
		                               &render_pass ) );

		struct RenderPassMapItem item;
		memset( &item, 0, sizeof( struct RenderPassMapItem ) );
		item.info        = *info;
		item.render_pass = render_pass;

		hashmap_set( hasher->render_passes, &item );

		return render_pass;
	}
}

VkFramebuffer
vk_pass_hasher_get_framebuffer( struct VulkanPassHasher*            hasher,
                                const struct VulkanFramebufferInfo* info )
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
		VkFramebufferCreateInfo create_info = {
			.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext           = NULL,
			.flags           = 0,
			.renderPass      = info->render_pass,
			.width           = info->width,
			.height          = info->height,
			.layers          = info->layers,
			.attachmentCount = info->attachment_count,
			.pAttachments    = info->attachments,
		};

		VkFramebuffer framebuffer;
		VK_ASSERT( vkCreateFramebuffer( hasher->device->logical_device,
		                                &create_info,
		                                hasher->device->vulkan_allocator,
		                                &framebuffer ) );

		struct FramebufferMapItem item;
		memset( &item, 0, sizeof( struct FramebufferMapItem ) );
		item.info        = *info;
		item.framebuffer = framebuffer;

		hashmap_set( hasher->framebuffers, &item );

		return framebuffer;
	}
}
