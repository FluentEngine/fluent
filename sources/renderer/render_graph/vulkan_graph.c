#include "log/log.h"
#include "render_graph_private.h"
#include "vulkan_graph.h"

#define APPEND_ARRAY_IF_NEED( ARR, COUNT, CAP, TYPE )                          \
	if ( COUNT == CAP - 1 )                                                    \
	{                                                                          \
		CAP *= 2;                                                              \
		ARR = realloc( ARR, CAP * sizeof( TYPE ) );                            \
	}

static inline void
vk_cmd_begin_render_pass( struct VulkanPassHasher*          pass_hasher,
                          const struct VulkanCommandBuffer* cmd,
                          const struct VulkanPassInfo*      info )
{
	VkRenderPass render_pass =
	    vk_pass_hasher_get_render_pass( pass_hasher, info );
	VkFramebuffer framebuffer =
	    vk_pass_hasher_get_framebuffer( pass_hasher, render_pass, info );

	static VkClearValue clear_values[ MAX_ATTACHMENTS_COUNT + 1 ];
	u32                 clear_value_count = info->color_attachment_count;

	for ( u32 i = 0; i < info->color_attachment_count; ++i )
	{
		VkClearValue clear_value = { 0 };
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
		VkClearValue clear_value = { 0 };
		clear_values[ idx ]      = clear_value;
		clear_values[ idx ].depthStencil.depth =
		    info->clear_values[ idx ].depth_stencil.depth;
		clear_values[ idx ].depthStencil.stencil =
		    info->clear_values[ idx ].depth_stencil.stencil;
	}

	VkRenderPassBeginInfo render_pass_begin_info = { 0 };
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

static inline void
vk_cmd_end_render_pass( const struct VulkanCommandBuffer* cmd )
{
	vkCmdEndRenderPass( cmd->command_buffer );
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

	for ( u32 p = 0; p < graph->pass_count; ++p )
	{
		const struct VulkanGraphPass* pass = graph->passes[ p ];
		struct VulkanPassInfo* physical_pass = &graph->physical_passes[ p ];

		physical_pass->color_attachment_count = pass->color_output_count;
		physical_pass->device                 = &graph->device->interface;

		for ( u32 att = 0; att < pass->color_output_count; ++att )
		{
			physical_pass->color_attachment_load_ops[ att ] =
			    FT_ATTACHMENT_LOAD_OP_CLEAR;
			physical_pass->color_image_initial_states[ att ] =
			    FT_RESOURCE_STATE_UNDEFINED;
			physical_pass->color_image_final_states[ att ] =
			    FT_RESOURCE_STATE_COLOR_ATTACHMENT;
			physical_pass->clear_values[ 0 ].color[ 0 ] = 0.0f;
			physical_pass->clear_values[ 0 ].color[ 1 ] = 0.0f;
			physical_pass->clear_values[ 0 ].color[ 2 ] = 0.0f;
			physical_pass->clear_values[ 0 ].color[ 3 ] = 1.0f;
		}
	}
}

static void
vk_rg_setup_attachments( struct RenderGraph* igraph,
                         const struct Image* iimage )
{
	FT_FROM_HANDLE( graph, igraph, VulkanGraph );
	FT_FROM_HANDLE( image, iimage, VulkanImage );

	graph->backbuffer_image = image;

	for ( u32 p = 0; p < graph->physical_pass_count; ++p )
	{
		struct VulkanPassInfo* pass = &graph->physical_passes[ p ];

		for ( u32 att = 0; att < pass->color_attachment_count; ++att )
		{
			pass->color_attachments[ att ] =
			    &graph->backbuffer_image->interface;
			pass->width  = pass->color_attachments[ att ]->width;
			pass->height = pass->color_attachments[ att ]->height;
		}
	}
}

static void
vk_rg_execute( struct RenderGraph* igraph, struct CommandBuffer* icmd )
{
	FT_FROM_HANDLE( graph, igraph, VulkanGraph );
	FT_FROM_HANDLE( cmd, icmd, VulkanCommandBuffer );

	for ( u32 p = 0; p < graph->physical_pass_count; ++p )
	{
		vk_cmd_begin_render_pass( &graph->pass_hasher,
		                          cmd,
		                          &graph->physical_passes[ p ] );
		vk_cmd_end_render_pass( cmd );
	}

	VkImageMemoryBarrier image_memory_barrier = {
		.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext               = NULL,
		.image               = graph->backbuffer_image->image,
		.oldLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.newLayout           = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.srcAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dstAccessMask       = VK_ACCESS_MEMORY_READ_BIT,
		.subresourceRange    = { .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
		                         .baseArrayLayer = 0,
		                         .baseMipLevel   = 0,
		                         .layerCount     = 1,
		                         .levelCount     = 1 },
	};

	vkCmdPipelineBarrier( cmd->command_buffer,
	                      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	                      VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
	                      0,
	                      0,
	                      NULL,
	                      0,
	                      NULL,
	                      1,
	                      &image_memory_barrier );
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
