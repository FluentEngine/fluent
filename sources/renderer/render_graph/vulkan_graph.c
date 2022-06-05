#include "log/log.h"
#include "render_graph_private.h"
#include "vulkan_graph.h"

static void
vk_rg_destroy( struct RenderGraph* igraph )
{
	FT_FROM_HANDLE( graph, igraph, VulkanGraph );

	free( graph );
}

static struct RenderGraphPass*
vk_rg_add_pass( struct RenderGraph* igraph, const char* pass_name )
{
	struct RenderGraphPass* p;
	FT_INIT_INTERNAL( pass, p, VulkanGraphPass );
	return p;
}

static void
vk_rg_add_color_output( struct RenderGraphPass* ipass,
                        const struct ImageInfo* image_info,
                        const char*             name )
{
}

static void
vk_rg_build( struct RenderGraph* igraph )
{
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

	VkImageMemoryBarrier image_memory_barrier = {
		.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext               = NULL,
		.image               = graph->backbuffer_image->image,
		.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout           = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.srcAccessMask       = VK_ACCESS_NONE,
		.dstAccessMask       = VK_ACCESS_NONE,
		.subresourceRange    = { .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
		                         .baseArrayLayer = 0,
		                         .baseMipLevel   = 0,
		                         .layerCount     = 1,
		                         .levelCount     = 1 },
	};

	vkCmdPipelineBarrier( cmd->command_buffer,
	                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	                      VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
	                      0,
	                      0,
	                      NULL,
	                      0,
	                      NULL,
	                      1,
	                      &image_memory_barrier );
}

void
vk_rg_create( const struct Device* idevice, struct RenderGraph** p )
{
	FT_INIT_INTERNAL( graph, *p, VulkanGraph );

	rg_destroy_impl           = vk_rg_destroy;
	rg_add_pass_impl          = vk_rg_add_pass;
	rg_add_color_output_impl  = vk_rg_add_color_output;
	rg_build_impl             = vk_rg_build;
	rg_setup_attachments_impl = vk_rg_setup_attachments;
	rg_execute_impl           = vk_rg_execute;
}
