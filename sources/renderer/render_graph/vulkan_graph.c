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
vk_rg_set_backbuffer_source( struct RenderGraph* igraph, const char* name )
{
	FT_FROM_HANDLE( graph, igraph, VulkanGraph );
	FT_UNUSED( rg_get_image( graph, name, &graph->backbuffer_image_index ) );
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
	graph->physical_passes     = calloc( graph->physical_pass_count,
                                     sizeof( struct VulkanPhysicalPass ) );

	for ( u32 p = 0; p < graph->physical_pass_count; ++p )
	{
		struct VulkanGraphPass*       graph_pass = graph->passes[ p ];
		struct VulkanPhysicalPass*    pass       = &graph->physical_passes[ p ];
		struct VulkanRenderPassInfo*  info       = &pass->render_pass_info;
		struct VulkanFramebufferInfo* fb_info    = &pass->framebuffer_info;
		struct VulkanSubpassInfo*     subpass_info = &info->subpasses[ 0 ];

		info->subpass_count                  = 1;
		subpass_info->color_attachment_count = graph_pass->color_output_count;
		info->attachment_count               = graph_pass->color_output_count;

		b32 swapchain_pass = 0;
				
		for ( u32 a = 0; a < info->attachment_count; ++a )
		{
			u32 image_index = graph_pass->color_outputs[ a ];

			if ( image_index == graph->backbuffer_image_index )
			{
				swapchain_pass = 1;
			}

			struct ImageInfo* image_info  = &graph->images[ image_index ].info;

			VkAttachmentDescription* att = &info->attachments[ a ];
			att->flags                   = 0;
			att->format                  = to_vk_format( image_info->format );
			att->samples       = to_vk_sample_count( image_info->sample_count );
			att->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			att->finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			att->loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR;
			att->storeOp       = VK_ATTACHMENT_STORE_OP_STORE;
			att->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			att->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

			subpass_info->color_attachment_references[ a ].attachment = a;
			subpass_info->color_attachment_references[ a ].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}

		fb_info->attachment_count = info->attachment_count;
		fb_info->render_pass =
		    vk_pass_hasher_get_render_pass( &graph->pass_hasher, info );
		if (swapchain_pass)
		{
			graph->swap_graph_pass_index = p;
			graph->swap_pass_index       = p;
		}
		else
		{
		}
	}
}

static void
vk_rg_setup_attachments( struct RenderGraph* igraph,
                         const struct Image* iimage )
{
	FT_FROM_HANDLE( graph, igraph, VulkanGraph );
	FT_FROM_HANDLE( backbuffer_image, iimage, VulkanImage );

	struct VulkanGraphPass* graph_pass =
	    graph->passes[ graph->swap_graph_pass_index ];
	struct VulkanPhysicalPass*    pass = &graph->physical_passes[ graph->swap_pass_index ];
	struct VulkanFramebufferInfo* info = &pass->framebuffer_info;

	for ( u32 a = 0; a < info->attachment_count; ++a )
	{
		info->width  = iimage->width;
		info->height = iimage->height;
		info->layers = iimage->layer_count;

		if ( graph_pass->color_outputs[ a ] == graph->backbuffer_image_index )
		{
			info->attachments[ a ] = backbuffer_image->image_view;
		}
	}

	pass->begin_info.sType      = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	pass->begin_info.pNext      = NULL;
	pass->begin_info.renderPass = pass->framebuffer_info.render_pass;
	pass->begin_info.framebuffer =
	    vk_pass_hasher_get_framebuffer( &graph->pass_hasher, info );
	pass->begin_info.clearValueCount          = 1;
	pass->begin_info.pClearValues             = pass->clear_values;
	pass->begin_info.renderArea.extent.width  = iimage->width;
	pass->begin_info.renderArea.extent.height = iimage->height;
}

static void
vk_rg_execute( struct RenderGraph* igraph, struct CommandBuffer* icmd )
{
	FT_FROM_HANDLE( graph, igraph, VulkanGraph );
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );

	for ( u32 p = 0; p < graph->physical_pass_count; ++p )
	{
		vkCmdBeginRenderPass( cmd->command_buffer,
		                      &graph->physical_passes[ p ].begin_info,
		                      VK_SUBPASS_CONTENTS_INLINE );

		vkCmdEndRenderPass( cmd->command_buffer );
	}
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

	rg_destroy_impl               = vk_rg_destroy;
	rg_add_pass_impl              = vk_rg_add_pass;
	rg_add_color_output_impl      = vk_rg_add_color_output;
	rg_set_backbuffer_source_impl = vk_rg_set_backbuffer_source;
	rg_build_impl                 = vk_rg_build;
	rg_setup_attachments_impl     = vk_rg_setup_attachments;
	rg_execute_impl               = vk_rg_execute;

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
