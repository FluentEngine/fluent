#include "log/log.h"
#include "render_graph_private.h"
#include "vulkan_graph.h"

#define APPEND_ARRAY_IF_NEED( ARR, COUNT, CAP, TYPE )                          \
	if ( COUNT == CAP - 1 )                                                    \
	{                                                                          \
		CAP *= 2;                                                              \
		ARR = realloc( ARR, CAP * sizeof( TYPE ) );                            \
	}

static inline struct VulkanGraphImage*
rg_get_image( struct VulkanGraph* graph, const char* name, u32* idx )
{
	struct NameToIndex* it =
	    hashmap_get( graph->image_name_to_index,
	                 &( struct NameToIndex ) { .name = name } );

	if ( it != NULL )
	{
		*idx = it->idx;
		return graph->images + it->idx;
	}
	else
	{
		hashmap_set( graph->image_name_to_index,
		             &( struct NameToIndex ) {
		                 .name = name,
		                 .idx  = graph->image_count,
		             } );

		APPEND_ARRAY_IF_NEED( graph->images,
		                      graph->image_count,
		                      graph->image_capacity,
		                      struct VulkanGraphImage );

		struct VulkanGraphImage* p = graph->images + graph->image_count;
		memset( p, 0, sizeof( struct VulkanGraphImage ) );
		*idx = graph->image_count;
		graph->image_count++;
		return p;
	}
}

static struct RenderGraphPass*
vk_rg_add_pass( struct RenderGraph* igraph, const char* name )
{
	FT_FROM_HANDLE( graph, igraph, VulkanGraph );

	struct RenderGraphPass* p;
	FT_INIT_INTERNAL( pass, p, VulkanGraphPass );
	pass->name               = name;
	pass->color_output_count = 0;
	pass->graph              = graph;

	APPEND_ARRAY_IF_NEED( graph->passes,
	                      graph->pass_count,
	                      graph->pass_capacity,
	                      struct VulkanGraphPass* );

	graph->passes[ graph->pass_count++ ] = pass;

	return p;
}

static void
vk_rg_add_color_output( struct RenderGraphPass* ipass,
                        const struct ImageInfo* info,
                        const char*             name )
{
	FT_FROM_HANDLE( pass, ipass, VulkanGraphPass );
	u32                      idx;
	struct VulkanGraphImage* image = rg_get_image( pass->graph, name, &idx );
	image->name                    = name;
	image->info                    = *info;
	image->info.descriptor_type    = FT_DESCRIPTOR_TYPE_COLOR_ATTACHMENT;

	pass->color_outputs[ pass->color_output_count++ ] = idx;
}

static void
vk_rg_build( struct RenderGraph* igraph )
{
	FT_FROM_HANDLE( graph, igraph, VulkanGraph );

	if ( graph->physical_passes != NULL )
	{
		free( graph->physical_passes );
	}

	graph->physical_pass_count = graph->pass_count;
	graph->physical_passes =
	    calloc( graph->physical_pass_count, sizeof( struct VulkanPassInfo ) );
}

static void
vk_rg_setup_attachments( struct RenderGraph* igraph,
                         const struct Image* iimage )
{
	FT_FROM_HANDLE( graph, igraph, VulkanGraph );
	FT_FROM_HANDLE( image, iimage, VulkanImage );

	graph->backbuffer_image = image;
}

static void
vk_rg_execute( struct RenderGraph* igraph, struct CommandBuffer* icmd )
{
	FT_FROM_HANDLE( graph, igraph, VulkanGraph );
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );
	
	const struct Image* image = &graph->backbuffer_image->interface;

	VkAttachmentDescription attachment_description = {
		.flags          = 0,
		.format         = to_vk_format( image->format ),
		.samples        = to_vk_sample_count( image->sample_count ),
		.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
	};

	VkAttachmentReference attachment_ref = {
		.attachment = 0,
		.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	VkSubpassDescription subpass_description = {
		.flags                   = 0,
		.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount    = 1,
		.pColorAttachments       = &attachment_ref,
		.pDepthStencilAttachment = NULL,
		.inputAttachmentCount    = 0,
		.pInputAttachments       = NULL,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments    = NULL,
		.pResolveAttachments     = NULL,
	};

	VkRenderPassCreateInfo render_pass_info = {
		.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext           = NULL,
		.flags           = 0,
		.attachmentCount = 1,
		.pAttachments    = &attachment_description,
		.subpassCount    = 1,
		.pSubpasses      = &subpass_description,
		.dependencyCount = 0,
		.pDependencies   = NULL,
	};
	
	VkRenderPass render_pass = vk_pass_hasher_get_render_pass(&graph->pass_hasher, &render_pass_info);

	VkFramebufferCreateInfo framebuffer_create_info = {
		.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.flags           = 0,
		.pNext           = NULL,
		.renderPass      = render_pass,
		.attachmentCount = 1,
		.pAttachments    = &graph->backbuffer_image->image_view,
		.width           = image->width,
		.height          = image->height,
		.layers          = 1,
	};

	VkFramebuffer framebuffer =
	    vk_pass_hasher_get_framebuffer( &graph->pass_hasher,
	                                    &framebuffer_create_info );

	VkClearValue clear_value = { 0 };

	VkRenderPassBeginInfo render_pass_begin_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = NULL,
		.renderPass = render_pass,
		.framebuffer = framebuffer,
		.clearValueCount = 1,
		.pClearValues = &clear_value,
		.renderArea = {
			.offset = {
				.x = 0,
				.y = 0
			},
			.extent = {
				.width = image->width,
				.height = image->height,
			}
		},
	};

	vkCmdBeginRenderPass( cmd->command_buffer,
	                      &render_pass_begin_info,
	                      VK_SUBPASS_CONTENTS_INLINE );
	                      
	vkCmdEndRenderPass( cmd->command_buffer );
}

static void
vk_rg_destroy( struct RenderGraph* igraph )
{
	FT_FROM_HANDLE( graph, igraph, VulkanGraph );

	vk_pass_hasher_shutdown( &graph->pass_hasher );

	if ( graph->physical_passes != NULL )
	{
		free( graph->physical_passes );
	}

	if ( graph->passes != NULL )
	{
		for ( u32 i = 0; i < graph->pass_count; ++i )
		{
			free( graph->passes[ i ] );
		}

		free( graph->passes );
	}

	if ( graph->images != NULL )
	{
		free( graph->images );
	}

	hashmap_free( graph->image_name_to_index );

	free( graph );
}

void
vk_rg_create( const struct Device* idevice, struct RenderGraph** p )
{
	FT_FROM_HANDLE( device, idevice, VulkanDevice );

	FT_INIT_INTERNAL( graph, *p, VulkanGraph );

	rg_destroy_impl           = vk_rg_destroy;
	rg_add_pass_impl          = vk_rg_add_pass;
	rg_add_color_output_impl  = vk_rg_add_color_output;
	rg_build_impl             = vk_rg_build;
	rg_setup_attachments_impl = vk_rg_setup_attachments;
	rg_execute_impl           = vk_rg_execute;

	graph->device = device;

	graph->image_count    = 0;
	graph->image_capacity = 1;
	graph->images         = NULL;

	graph->image_name_to_index = create_name_to_index_map();

	graph->pass_count    = 0;
	graph->pass_capacity = 1;
	graph->passes        = NULL;

	graph->physical_pass_count = 0;
	graph->physical_passes     = NULL;

	vk_pass_hasher_init( &graph->pass_hasher, graph->device );
}
